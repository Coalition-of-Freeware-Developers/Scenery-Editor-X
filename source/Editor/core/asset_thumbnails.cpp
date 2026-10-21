/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * asset_thumbnails.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#include "asset_thumbnails.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <future>
#include <mutex>
#include <span>
#include <unordered_map>
#include <vector>
#include <stb_image_write.h>
#include <SceneryEditorX/core/frame_profiler.h>
#include <SceneryEditorX/core/threading/thread_pool.h>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/ui/ui_impl.h>
#include <SceneryEditorX/renderer/vulkan/image_resource.h>
#include <SceneryEditorX/xplane/xp_library_manager.h>
#include <SceneryEditorX/xplane/xp_texture_loader.h>
#include <SceneryEditorX/xplane/xp_thumbnail.h>

// -------------------------------------------------------

namespace SceneryEditorX::AssetThumbnails
{
	namespace fs = std::filesystem;

	namespace
	{
		constexpr const char *LOG_TAG = "Thumbnails";
		constexpr uint32_t SMALL_SIZE = 128;
		constexpr uint32_t LARGE_SIZE = 256;
		constexpr uint32_t PAGE_SIZE = 1024;
		constexpr uint32_t CELLS_PER_ROW = PAGE_SIZE / SMALL_SIZE;
		constexpr uint32_t CELLS_PER_PAGE = CELLS_PER_ROW * CELLS_PER_ROW;
		constexpr size_t MAX_PAGES = 6;					// 384 small thumbnails on the GPU, 24 MB
		constexpr size_t MAX_LARGE = 4;
		constexpr int MAX_WORKERS = 2;
		constexpr uint64_t STALE_FRAMES = 30;			// A queued request nobody drew for this long is dropped
		constexpr double UPLOAD_BUDGET_MS = 3.0;

		struct Request
		{
			std::string key;
			fs::path file;
			fs::path cacheFile;
			Size size = Size::Small;
			std::shared_ptr<const XPlane::LibraryIndex> library;
		};

		struct Result
		{
			std::string key;
			XPlane::RgbaImage image;
			std::string error;
			bool fromDisk = false;
			double milliseconds = 0.0;
		};

		struct Entry
		{
			State state = State::Pending;
			Size size = Size::Small;
			int page = -1;							// Small: atlas page and cell
			uint32_t cell = 0;
			Ref<ImageResource> image;				// Large
			std::string error;
			uint64_t lastUsed = 0;
		};

		struct Page
		{
			Ref<ImageResource> image;
			std::array<std::string, CELLS_PER_PAGE> owners;	// Entry key per cell, empty when free
		};

		struct ThumbnailState
		{
			std::mutex mutex;						// queue, results, workers
			std::deque<Request> queue;				// Newest at the back: served first
			std::vector<Result> results;
			int workers = 0;
			std::vector<std::future<void>> futures;
			std::atomic<bool> stopping{ false };

			std::unordered_map<std::string, Entry> entries;
			std::vector<Page> pages;
			uint64_t frame = 0;
			uint64_t libraryGeneration = ~0ull;
			Stats stats;
		};

		ThumbnailState &S()
		{
			static ThumbnailState state;
			return state;
		}

		std::string EntryKey(const fs::path &file, const Size size)
		{
			return (size == Size::Large ? "L|" : "S|") + VFS::ToUtf8(file.lexically_normal());
		}

		uint64_t Fnv1a(const std::string &text, uint64_t hash = 14695981039346656037ull)
		{
			for (const char c : text)
			{
				hash ^= static_cast<uint8_t>(c);
				hash *= 1099511628211ull;
			}
			return hash;
		}

		/* @brief <cache>/thumbnails/<hash>.png: the path, size and time of the file, the thumbnail version and pixel size. */
		fs::path CacheFile(const fs::path &file, const uint32_t pixels)
		{
			std::error_code ec;
			const uintmax_t bytes = fs::file_size(file, ec);
			const auto time = fs::last_write_time(file, ec);
			const std::string stamp = VFS::ToUtf8(file.lexically_normal()) + "|" + std::to_string(ec ? 0 : bytes) + "|" +
				std::to_string(ec ? 0 : time.time_since_epoch().count()) + "|" + std::to_string(XPlane::THUMBNAIL_VERSION) + "|" + std::to_string(pixels);
			char name[32];
			std::snprintf(name, sizeof(name), "%016llx.png", static_cast<unsigned long long>(Fnv1a(stamp)));
			return VFS::GetAppLocation(VFS::AppLocation::Cache) / "thumbnails" / name;
		}

		bool LoadTexture(const fs::path &file, const uint32_t minSize, XPlane::RgbaImage &out)
		{
			std::string extension = VFS::ToUtf8(file.extension());
			std::transform(extension.begin(), extension.end(), extension.begin(), [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
			if (extension == ".dds")
			{
				std::string bytes;
				if (!VFS::ReadFile(file, bytes))
					return false;
				if (!XPlane::DecodeDds(std::span(reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size()), minSize, out))
					return false;
				XPlane::FlipRows(out);	// Row 0 at the bottom
				return true;
			}
			XPlane::CpuImage image;
			if (!XPlane::TextureLoader::DecodeRGBA(file, image) || !image.IsValid())
				return false;
			out.width = image.width;
			out.height = image.height;
			out.pixels = std::move(image.pixels);
			XPlane::ShrinkImage(out, std::max(minSize * 2, 64u));
			return true;
		}

		bool ReadCachedPng(const fs::path &file, const uint32_t pixels, XPlane::RgbaImage &out)
		{
			std::string bytes;
			std::error_code ec;
			if (!fs::exists(file, ec) || !VFS::ReadFile(file, bytes))
				return false;
			int width = 0, height = 0, channels = 0;
			stbi_uc *data = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(bytes.data()), static_cast<int>(bytes.size()), &width, &height, &channels, 4);
			if (!data)
				return false;
			const bool fits = width == static_cast<int>(pixels) && height == static_cast<int>(pixels);
			if (fits)
			{
				out.width = static_cast<uint32_t>(width);
				out.height = static_cast<uint32_t>(height);
				out.pixels.assign(data, data + static_cast<size_t>(width) * height * 4);
			}
			stbi_image_free(data);
			return fits;
		}

		void WriteCachedPng(const fs::path &file, const XPlane::RgbaImage &image)
		{
			std::error_code ec;
			fs::create_directories(file.parent_path(), ec);
			std::string png;
			const auto append = [](void *context, void *data, const int size) { static_cast<std::string *>(context)->append(static_cast<const char *>(data), static_cast<size_t>(size)); };
			if (stbi_write_png_to_func(append, &png, static_cast<int>(image.width), static_cast<int>(image.height), 4, image.pixels.data(), static_cast<int>(image.width * 4)) && !png.empty())
				VFS::WriteFileAtomic(file, png);
		}

		Result Process(const Request &request, XPlane::ThumbnailBuilder &builder)
		{
			const auto start = std::chrono::steady_clock::now();
			Result result;
			result.key = request.key;
			const uint32_t pixels = request.size == Size::Large ? LARGE_SIZE : SMALL_SIZE;
			if (ReadCachedPng(request.cacheFile, pixels, result.image))
				result.fromDisk = true;
			else
			{
				XPlane::ThumbnailOptions options;
				options.size = pixels;
				if (builder.Render(request.file, options, result.image, &result.error))
					WriteCachedPng(request.cacheFile, result.image);
				else
					result.image = {};
			}
			result.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			return result;
		}

		/* @brief A worker: serves the queue newest first until it is empty. Thumbnails of one library share a builder. */
		void RunWorker()
		{
			ThumbnailState &state = S();
			std::unique_ptr<XPlane::ThumbnailBuilder> builder;
			const XPlane::LibraryIndex *builderLibrary = nullptr;
			std::shared_ptr<const XPlane::LibraryIndex> library;
			while (true)
			{
				Request request;
				{
					std::lock_guard lock(state.mutex);
					if (state.stopping || state.queue.empty())
					{
						--state.workers;
						return;
					}
					request = std::move(state.queue.back());
					state.queue.pop_back();
				}
				if (!builder || request.library.get() != builderLibrary)
				{
					library = request.library;
					builderLibrary = library.get();
					builder = std::make_unique<XPlane::ThumbnailBuilder>([library](const std::string_view path) -> fs::path
					{
						return library ? library->Resolve(path) : fs::path();
					}, LoadTexture);
				}
				Result result = Process(request, *builder);
				std::lock_guard lock(state.mutex);
				state.results.push_back(std::move(result));
			}
		}

		void StartWorkers()
		{
			ThumbnailState &state = S();
			std::lock_guard lock(state.mutex);
			std::erase_if(state.futures, [](const std::future<void> &future) { return future.wait_for(std::chrono::seconds(0)) == std::future_status::ready; });
			while (!state.stopping && state.workers < MAX_WORKERS && static_cast<size_t>(state.workers) < state.queue.size())
			{
				++state.workers;
				state.futures.push_back(ThreadPool::Submit([] { RunWorker(); }));
			}
		}

		void FreeCell(const Entry &entry)
		{
			ThumbnailState &state = S();
			if (entry.page >= 0 && static_cast<size_t>(entry.page) < state.pages.size())
				state.pages[static_cast<size_t>(entry.page)].owners[entry.cell].clear();
		}

		/* @brief A free atlas cell: a new page while there is room, else the least recently drawn thumbnail's. */
		bool AllocateCell(const std::string &key, int &page, uint32_t &cell)
		{
			ThumbnailState &state = S();
			for (size_t p = 0; p < state.pages.size(); ++p)
			{
				for (uint32_t c = 0; c < CELLS_PER_PAGE; ++c)
				{
					if (state.pages[p].owners[c].empty())
					{
						state.pages[p].owners[c] = key;
						page = static_cast<int>(p);
						cell = c;
						return true;
					}
				}
			}
			if (state.pages.size() < MAX_PAGES)
			{
				Page fresh;
				std::vector<Slice> slices(1);
				slices[0].mips.resize(1);
				slices[0].mips[0].bytes.assign(static_cast<size_t>(PAGE_SIZE) * PAGE_SIZE * 4, std::byte{ 0 });
				fresh.image = CreateRef<ImageResource>(ImgResourceSpec{
					.type = ImageType::Type2D,
					.width = PAGE_SIZE,
					.height = PAGE_SIZE,
					.depth = 1,
					.mipCount = 1,
					.format = VK_FORMAT_R8G8B8A8_SRGB,
					.flags = ShaderViews | Transparent,
					.name = "Thumbnail atlas" }, std::move(slices));
				if (!fresh.image)
					return false;
				fresh.owners[0] = key;
				state.pages.push_back(std::move(fresh));
				page = static_cast<int>(state.pages.size() - 1);
				cell = 0;
				return true;
			}

			// Evict the least recently drawn (not this frame's)
			Entry *oldest = nullptr;
			for (auto &[entryKey, entry] : state.entries)
			{
				if (entry.state == State::Ready && entry.page >= 0 && entry.lastUsed < state.frame && (!oldest || entry.lastUsed < oldest->lastUsed))
					oldest = &entry;
			}
			if (!oldest)
				return false;
			page = oldest->page;
			cell = oldest->cell;
			state.pages[static_cast<size_t>(page)].owners[cell] = key;
			oldest->state = State::Pending;		// Asked for again: back from the disk cache
			oldest->page = -1;
			for (auto it = state.entries.begin(); it != state.entries.end(); ++it)
			{
				if (&it->second == oldest)
				{
					state.entries.erase(it);
					break;
				}
			}
			return true;
		}

		void Queue(const std::string &key, const fs::path &file, const Size size)
		{
			ThumbnailState &state = S();
			Request request;
			request.key = key;
			request.file = file;
			request.size = size;
			request.cacheFile = CacheFile(file, size == Size::Large ? LARGE_SIZE : SMALL_SIZE);
			request.library = XPlane::LibraryManager::GetIndex();
			std::lock_guard lock(state.mutex);
			state.queue.push_back(std::move(request));
		}
	}

	// -------------------------------------------------------

	State Get(const fs::path &file, Thumbnail &out, const Size size)
	{
		out = {};
		const std::string name = VFS::ToUtf8(file);
		if (file.empty() || !XPlane::CanMakeThumbnail(name))
			return State::Unsupported;

		ThumbnailState &state = S();
		if (state.stopping)
			return State::Unsupported;
		const std::string key = EntryKey(file, size);
		auto it = state.entries.find(key);
		if (it == state.entries.end())
		{
			it = state.entries.emplace(key, Entry{}).first;
			it->second.size = size;
			Queue(key, file, size);
		}
		Entry &entry = it->second;
		entry.lastUsed = state.frame;
		if (entry.state != State::Ready)
			return entry.state;

		if (entry.size == Size::Large)
			out.texture = entry.image ? UI::GetTextureID(entry.image.Get()) : ImTextureID_Invalid;
		else if (entry.page >= 0 && static_cast<size_t>(entry.page) < state.pages.size())
		{
			out.texture = UI::GetTextureID(state.pages[static_cast<size_t>(entry.page)].image.Get());
			const float cellUv = static_cast<float>(SMALL_SIZE) / static_cast<float>(PAGE_SIZE);
			out.uv0 = ImVec2(static_cast<float>(entry.cell % CELLS_PER_ROW) * cellUv, static_cast<float>(entry.cell / CELLS_PER_ROW) * cellUv);
			out.uv1 = ImVec2(out.uv0.x + cellUv, out.uv0.y + cellUv);
		}
		return out.texture != ImTextureID_Invalid ? State::Ready : State::Pending;
	}

	bool Draw(const fs::path &file, const ImVec2 &min, const ImVec2 &max, const Size size)
	{
		Thumbnail thumbnail;
		if (Get(file, thumbnail, size) != State::Ready)
			return false;
		const float side = std::min(max.x - min.x, max.y - min.y);
		const ImVec2 corner(min.x + (max.x - min.x - side) * 0.5f, min.y + (max.y - min.y - side) * 0.5f);
		ImGui::GetWindowDrawList()->AddImage(thumbnail.texture, corner, ImVec2(corner.x + side, corner.y + side), thumbnail.uv0, thumbnail.uv1);
		return true;
	}

	std::string GetError(const fs::path &file, const Size size)
	{
		const auto it = S().entries.find(EntryKey(file, size));
		return it != S().entries.end() && it->second.state == State::Failed ? it->second.error : std::string();
	}

	void Invalidate(const fs::path &file)
	{
		ThumbnailState &state = S();
		for (const Size size : { Size::Small, Size::Large })
		{
			const auto it = state.entries.find(EntryKey(file, size));
			if (it == state.entries.end() || it->second.state == State::Pending)
				continue;
			FreeCell(it->second);
			state.entries.erase(it);
		}
	}

	void Update()
	{
		ThumbnailState &state = S();
		if (state.stopping)
			return;
		SEDX_PERF_SCOPE("Main: thumbnails");
		++state.frame;

		// A newer library index resolves differently: draw what is asked for again (the disk cache keeps the files)
		if (const uint64_t generation = XPlane::LibraryManager::GetGeneration(); generation != state.libraryGeneration)
		{
			if (state.libraryGeneration != ~0ull)
			{
				std::erase_if(state.entries, [](const auto &item) { return item.second.state == State::Failed; });
			}
			state.libraryGeneration = generation;
		}

		// Requests nobody drew lately leave the queue (scrolled past)
		std::vector<Result> results;
		{
			std::lock_guard lock(state.mutex);
			std::erase_if(state.queue, [&](const Request &request)
			{
				const auto it = state.entries.find(request.key);
				if (it != state.entries.end() && it->second.lastUsed + STALE_FRAMES >= state.frame)
					return false;
				if (it != state.entries.end())
					state.entries.erase(it);
				return true;
			});
			results.swap(state.results);
		}

		// Upload the finished ones under a time budget; the rest wait for the next frame
		const auto start = std::chrono::steady_clock::now();
		std::vector<XPlane::RgbaImage> keep;	// Pixels of this frame's atlas writes
		std::vector<std::vector<ImageResource::RegionWrite>> writes;
		std::vector<std::string> written;
		size_t used = 0;
		for (; used < results.size(); ++used)
		{
			if (std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() > UPLOAD_BUDGET_MS)
				break;
			Result &result = results[used];
			const auto it = state.entries.find(result.key);
			if (it == state.entries.end())
				continue;
			Entry &entry = it->second;
			if (result.fromDisk)
				++state.stats.fromDisk;
			else
			{
				++state.stats.rendered;
				state.stats.renderMilliseconds += result.milliseconds;
			}
			if (!result.image.IsValid())
			{
				entry.state = State::Failed;
				entry.error = result.error.empty() ? "No thumbnail" : result.error;
				continue;
			}

			if (entry.size == Size::Large)
			{
				// Its own image; only the last few are kept
				std::vector<Slice> slices(1);
				slices[0].mips.resize(1);
				slices[0].mips[0].bytes.resize(result.image.pixels.size());
				std::memcpy(slices[0].mips[0].bytes.data(), result.image.pixels.data(), result.image.pixels.size());
				entry.image = CreateRef<ImageResource>(ImgResourceSpec{
					.type = ImageType::Type2D,
					.width = result.image.width,
					.height = result.image.height,
					.depth = 1,
					.mipCount = 1,
					.format = VK_FORMAT_R8G8B8A8_SRGB,
					.flags = ShaderViews | Transparent,
					.name = "Thumbnail" }, std::move(slices));
				entry.state = entry.image ? State::Ready : State::Failed;
				std::vector<std::pair<uint64_t, std::string>> large;
				for (const auto &[key, other] : state.entries)
				{
					if (other.size == Size::Large && other.state == State::Ready)
						large.emplace_back(other.lastUsed, key);
				}
				if (large.size() > MAX_LARGE)
				{
					std::sort(large.begin(), large.end());
					for (size_t i = 0; i + MAX_LARGE < large.size(); ++i)
						state.entries.erase(large[i].second);
				}
				continue;
			}

			int page = -1;
			uint32_t cell = 0;
			if (!AllocateCell(result.key, page, cell))
				break;	// Every cell is on screen: try again next frame
			const auto again = state.entries.find(result.key);	// Eviction may have rehashed the map
			if (again == state.entries.end())
				continue;
			again->second.page = page;
			again->second.cell = cell;
			if (writes.size() <= static_cast<size_t>(page))
				writes.resize(static_cast<size_t>(page) + 1);
			keep.push_back(std::move(result.image));
			ImageResource::RegionWrite write;
			write.x = (cell % CELLS_PER_ROW) * SMALL_SIZE;
			write.y = (cell / CELLS_PER_ROW) * SMALL_SIZE;
			write.width = SMALL_SIZE;
			write.height = SMALL_SIZE;
			writes[static_cast<size_t>(page)].push_back(write);
			written.push_back(result.key);
		}

		// Texel pointers into keep, which no longer grows
		size_t next = 0;
		for (size_t page = 0; page < writes.size(); ++page)
		{
			for (ImageResource::RegionWrite &write : writes[page])
				write.texels = keep[next++].pixels.data();
		}
		bool uploaded = true;
		for (size_t page = 0; page < writes.size(); ++page)
		{
			if (!writes[page].empty())
				uploaded = state.pages[page].image->WriteRegions(writes[page]) && uploaded;
		}
		for (const std::string &key : written)
		{
			if (const auto it = state.entries.find(key); it != state.entries.end())
			{
				it->second.state = uploaded ? State::Ready : State::Failed;
				if (!uploaded)
				{
					it->second.error = "The thumbnail couldn't be uploaded";
					FreeCell(it->second);
					it->second.page = -1;
				}
			}
		}

		// What didn't fit in the budget goes back for the next frame
		if (used < results.size())
		{
			std::lock_guard lock(state.mutex);
			state.results.insert(state.results.begin(), std::make_move_iterator(results.begin() + static_cast<ptrdiff_t>(used)), std::make_move_iterator(results.end()));
		}
		StartWorkers();
	}

	void Shutdown()
	{
		ThumbnailState &state = S();
		{
			std::lock_guard lock(state.mutex);
			state.stopping = true;
			state.queue.clear();
		}
		for (std::future<void> &future : state.futures)
		{
			if (future.valid())
				future.wait();
		}
		state.futures.clear();
		state.results.clear();
		state.entries.clear();
		state.pages.clear();
		EDITOR_INFO_TAG(LOG_TAG, "Thumbnails: {} rendered ({:.0f} ms), {} from the disk cache", state.stats.rendered, state.stats.renderMilliseconds, state.stats.fromDisk);
	}

	Stats GetStats()
	{
		ThumbnailState &state = S();
		Stats stats = state.stats;
		for (const auto &[key, entry] : state.entries)
		{
			stats.ready += entry.state == State::Ready ? 1 : 0;
			stats.pending += entry.state == State::Pending ? 1 : 0;
			stats.failed += entry.state == State::Failed ? 1 : 0;
		}
		return stats;
	}

}

// -------------------------------------------------------
