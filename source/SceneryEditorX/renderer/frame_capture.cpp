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
 * frame_capture.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "frame_capture.h"
#include "vulkan/buffer.h"
#include "vulkan/command_list.h"
#include "vulkan/image_resource.h"
#include "vulkan/memory_allocator.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <mutex>
#include <optional>
#include <vector>
#include <SceneryEditorX/logging/logging.hpp>
#include <stb_image_write.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		constexpr const char *LOG_TAG = "Capture";

		struct Pending
		{
			std::filesystem::path png;
			bool skyMask = true;
		};

		struct InFlight
		{
			const CommandList *list = nullptr;
			Pending request;
			Ref<Buffer> color;
			Ref<Buffer> depth;
			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t depthWidth = 0;
			uint32_t depthHeight = 0;
			VkFormat colorFormat = VK_FORMAT_UNDEFINED;
			VkDeviceSize rawBytes = 0;				// RequestDump(): the texels go to request.png as they are
			bool sameRecording = false;				// RecordDump() ran earlier in the recording Record() is part of
		};

		struct State
		{
			std::mutex mutex;
			std::optional<Pending> queued;
			std::optional<std::filesystem::path> queuedDump;
			std::vector<InFlight> inFlight;
			std::optional<FrameCapture::Result> result;
		};

		State &S()
		{
			static State state;
			return state;
		}

		/* @brief Host-cached, persistently mapped memory the CPU reads after the GPU copied into it. */
		Ref<Buffer> CreateReadback(const VkDeviceSize bytes)
		{
			VmaAllocationCreateInfo allocInfo{};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			return CreateRef<Buffer>(MemoryAllocator::GetAllocator(), bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, allocInfo);
		}

		const void *MapReadback(const Ref<Buffer> &buffer)
		{
			if (!buffer)
				return nullptr;
			if (VmaAllocation allocation = buffer->Allocation())
				vmaInvalidateAllocation(MemoryAllocator::GetAllocator(), allocation, 0, VK_WHOLE_SIZE);
			return buffer->GetMappedData() ? buffer->GetMappedData() : buffer->Map();
		}

		bool IsUsable(ImageResource *image)
		{
			return image && image->Get() && *image->Get() != VK_NULL_HANDLE;
		}

		float HalfToFloat(const uint16_t h)
		{
			const uint32_t sign = (h & 0x8000u) << 16;
			const uint32_t exponent = (h >> 10) & 0x1Fu;
			const uint32_t mantissa = h & 0x3FFu;
			uint32_t bits;
			if (exponent == 0)
			{
				if (mantissa == 0)
					bits = sign;
				else
				{
					// Subnormal: normalize
					uint32_t e = 127 - 15 + 1;
					uint32_t m = mantissa;
					while ((m & 0x400u) == 0)
					{
						m <<= 1;
						--e;
					}
					bits = sign | (e << 23) | ((m & 0x3FFu) << 13);
				}
			}
			else if (exponent == 31)
				bits = sign | 0x7F800000u | (mantissa << 13);
			else
				bits = sign | ((exponent + 127 - 15) << 23) | (mantissa << 13);
			float value;
			std::memcpy(&value, &bits, sizeof(value));
			return value;
		}

		/* @brief Linear 0-1 -> 8-bit sRGB (IEC 61966-2-1), what the sRGB swapchain shows for the same value. */
		uint8_t EncodeSrgb(float linear)
		{
			if (!(linear > 0.0f))	// NaN and negatives
				return 0;
			linear = std::min(linear, 1.0f);
			const float encoded = linear <= 0.0031308f ? linear * 12.92f : 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
			return static_cast<uint8_t>(std::lround(std::clamp(encoded, 0.0f, 1.0f) * 255.0f));
		}

		FrameCapture::Result Write(const InFlight &flight)
		{
			FrameCapture::Result result;
			result.png = flight.request.png;

			const void *colorData = MapReadback(flight.color);
			if (!colorData)
			{
				result.message = "could not map the colour readback";
				return result;
			}

			const size_t pixels = static_cast<size_t>(flight.width) * flight.height;
			std::vector<uint8_t> rgb(pixels * 3);
			if (flight.colorFormat == VK_FORMAT_R16G16B16A16_SFLOAT)
			{
				const auto *half = static_cast<const uint16_t *>(colorData);
				for (size_t i = 0; i < pixels; ++i)
					for (int c = 0; c < 3; ++c)
						rgb[i * 3 + c] = EncodeSrgb(HalfToFloat(half[i * 4 + c]));
			}
			else if (flight.colorFormat == VK_FORMAT_R32G32B32A32_SFLOAT)
			{
				const auto *values = static_cast<const float *>(colorData);
				for (size_t i = 0; i < pixels; ++i)
					for (int c = 0; c < 3; ++c)
						rgb[i * 3 + c] = EncodeSrgb(values[i * 4 + c]);
			}
			else
			{
				// 8-bit UNORM: already display values (an sRGB-format target would be encoded by its view)
				const auto *bytes = static_cast<const uint8_t *>(colorData);
				const bool bgra = flight.colorFormat == VK_FORMAT_B8G8R8A8_UNORM || flight.colorFormat == VK_FORMAT_B8G8R8A8_SRGB;
				for (size_t i = 0; i < pixels; ++i)
					for (int c = 0; c < 3; ++c)
						rgb[i * 3 + c] = bytes[i * 4 + (bgra ? 2 - c : c)];
			}

			std::error_code ec;
			if (flight.request.png.has_parent_path())
				std::filesystem::create_directories(flight.request.png.parent_path(), ec);
			if (!stbi_write_png(flight.request.png.string().c_str(), static_cast<int>(flight.width), static_cast<int>(flight.height), 3, rgb.data(), static_cast<int>(flight.width * 3)))
			{
				result.message = "could not write " + flight.request.png.string();
				return result;
			}

			if (flight.depth)
			{
				if (const auto *depth = static_cast<const float *>(MapReadback(flight.depth)))
				{
					// Reverse-Z clears to 0: nothing was drawn there (sky). Sampled at the output size.
					std::vector<uint8_t> mask(pixels);
					for (uint32_t y = 0; y < flight.height; ++y)
					{
						const uint32_t dy = std::min(flight.depthHeight - 1, y * flight.depthHeight / flight.height);
						for (uint32_t x = 0; x < flight.width; ++x)
						{
							const uint32_t dx = std::min(flight.depthWidth - 1, x * flight.depthWidth / flight.width);
							mask[static_cast<size_t>(y) * flight.width + x] = depth[static_cast<size_t>(dy) * flight.depthWidth + dx] == 0.0f ? 255 : 0;
						}
					}
					std::filesystem::path maskPath = flight.request.png;
					maskPath.replace_filename(flight.request.png.stem().string() + "_sky.png");
					if (stbi_write_png(maskPath.string().c_str(), static_cast<int>(flight.width), static_cast<int>(flight.height), 1, mask.data(), static_cast<int>(flight.width)))
						result.skyMask = maskPath;
				}
			}

			result.success = true;
			result.message = std::to_string(flight.width) + " x " + std::to_string(flight.height);
			return result;
		}

		void HostReadBarrier(CommandList *cmdList)
		{
			VkMemoryBarrier2 barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
			barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			barrier.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
			barrier.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;

			VkDependencyInfo dependency{};
			dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependency.memoryBarrierCount = 1;
			dependency.pMemoryBarriers = &barrier;
			vkCmdPipelineBarrier2(cmdList->GetCommandBuffer(), &dependency);
		}

		uint32_t BytesPerTexel(const VkFormat format)
		{
			switch (format)
			{
				case VK_FORMAT_R16G16B16A16_SFLOAT:	return 8;
				case VK_FORMAT_R32G32B32A32_SFLOAT:	return 16;
				case VK_FORMAT_R32G32_SFLOAT:		return 8;
				case VK_FORMAT_R16G16_SFLOAT:		return 4;
				case VK_FORMAT_R8G8B8A8_UNORM:
				case VK_FORMAT_R8G8B8A8_SRGB:
				case VK_FORMAT_B8G8R8A8_UNORM:
				case VK_FORMAT_B8G8R8A8_SRGB:		return 4;
				default:							return 0;
			}
		}
	}

	// -------------------------------------------------------

	void FrameCapture::Request(const std::filesystem::path &png, const bool writeSkyMask)
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		s.queued = Pending{ png, writeSkyMask };
	}

	bool FrameCapture::IsBusy()
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		return s.queued.has_value() || !s.inFlight.empty();
	}

	bool FrameCapture::TakeResult(Result &out)
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		if (!s.result)
			return false;
		out = std::move(*s.result);
		s.result.reset();
		return true;
	}

	void FrameCapture::Record(CommandList *cmdList, ImageResource *output, ImageResource *depth)
	{
		if (!cmdList || cmdList->IsStickyInvalid())
			return;

		State &s = S();
		std::lock_guard lock(s.mutex);

		// This command list is being recorded again, so what it copied last time has completed
		for (auto it = s.inFlight.begin(); it != s.inFlight.end();)
		{
			if (it->list != cmdList)
			{
				++it;
				continue;
			}
			if (it->sameRecording)
			{
				// Copied earlier in this frame's recording: complete the next time this list comes round
				it->sameRecording = false;
				++it;
				continue;
			}
			if (it->rawBytes > 0)
			{
				// RequestDump(): no PNG, no result for the scenario runner
				const void *data = MapReadback(it->color);
				std::error_code ec;
				if (it->request.png.has_parent_path())
					std::filesystem::create_directories(it->request.png.parent_path(), ec);
				std::ofstream file(it->request.png, std::ios::binary);
				if (data && file.write(static_cast<const char *>(data), static_cast<std::streamsize>(it->rawBytes)))
					SEDX_CORE_INFO_TAG(LOG_TAG, "Image dumped to {} ({} x {}, {} bytes)", it->request.png.string(), it->width, it->height, it->rawBytes);
				else
					SEDX_CORE_ERROR_TAG(LOG_TAG, "Could not write the image dump {}", it->request.png.string());
				if (it->color)
					it->color->Destroy();
				it = s.inFlight.erase(it);
				continue;
			}
			Result result = Write(*it);
			if (result.success)
				SEDX_CORE_INFO_TAG(LOG_TAG, "Frame written to {} ({})", result.png.string(), result.message);
			else
				SEDX_CORE_ERROR_TAG(LOG_TAG, "Frame capture failed: {}", result.message);
			s.result = std::move(result);
			if (it->color)
				it->color->Destroy();
			if (it->depth)
				it->depth->Destroy();
			it = s.inFlight.erase(it);
		}

		if (!s.queued || !IsUsable(output))
			return;

		const Pending request = *s.queued;
		s.queued.reset();

		InFlight flight;
		flight.list = cmdList;
		flight.request = request;
		flight.width = output->GetWidth();
		flight.height = output->GetHeight();
		flight.colorFormat = output->GetImageSpec().format;
		const uint32_t texelBytes = BytesPerTexel(flight.colorFormat);
		if (texelBytes == 0 || flight.width == 0 || flight.height == 0)
		{
			s.result = Result{ false, request.png, {}, "unsupported output format or size" };
			return;
		}

		flight.color = CreateReadback(static_cast<VkDeviceSize>(flight.width) * flight.height * texelBytes);
		if (!flight.color || !flight.color->Get())
		{
			s.result = Result{ false, request.png, {}, "could not allocate a readback buffer" };
			return;
		}

		output->SetLayout(Layout::ImageLayout::TransferSrc, cmdList, 0, 0);
		VkBufferImageCopy copy{};
		copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.imageSubresource.layerCount = 1;
		copy.imageExtent = { flight.width, flight.height, 1 };
		vkCmdCopyImageToBuffer(cmdList->GetCommandBuffer(), *output->Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, flight.color->Get(), 1, &copy);

		if (request.skyMask && IsUsable(depth) && depth->GetImageSpec().format == VK_FORMAT_D32_SFLOAT)
		{
			flight.depthWidth = depth->GetWidth();
			flight.depthHeight = depth->GetHeight();
			flight.depth = CreateReadback(static_cast<VkDeviceSize>(flight.depthWidth) * flight.depthHeight * sizeof(float));
			if (flight.depth && flight.depth->Get())
			{
				depth->SetLayout(Layout::ImageLayout::TransferSrc, cmdList, 0, 0);
				VkBufferImageCopy depthCopy{};
				depthCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				depthCopy.imageSubresource.layerCount = 1;
				depthCopy.imageExtent = { flight.depthWidth, flight.depthHeight, 1 };
				vkCmdCopyImageToBuffer(cmdList->GetCommandBuffer(), *depth->Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, flight.depth->Get(), 1, &depthCopy);
			}
			else
				flight.depth = nullptr;
		}

		HostReadBarrier(cmdList);
		s.inFlight.push_back(std::move(flight));
	}

	void FrameCapture::RequestDump(const std::filesystem::path &file)
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		s.queuedDump = file;
	}

	void FrameCapture::RecordDump(CommandList *cmdList, ImageResource *image)
	{
		if (!cmdList || cmdList->IsStickyInvalid())
			return;

		State &s = S();
		std::lock_guard lock(s.mutex);
		if (!s.queuedDump || !IsUsable(image))
			return;

		InFlight flight;
		flight.list = cmdList;
		flight.request.png = *s.queuedDump;
		s.queuedDump.reset();
		flight.width = image->GetWidth();
		flight.height = image->GetHeight();
		flight.colorFormat = image->GetImageSpec().format;
		const uint32_t texelBytes = BytesPerTexel(flight.colorFormat);
		if (texelBytes == 0)
		{
			SEDX_CORE_ERROR_TAG(LOG_TAG, "Image dump {}: unsupported format", flight.request.png.string());
			return;
		}
		flight.rawBytes = static_cast<VkDeviceSize>(flight.width) * flight.height * texelBytes;
		flight.sameRecording = true;	// Record() runs later in this frame
		flight.color = CreateReadback(flight.rawBytes);
		if (!flight.color || !flight.color->Get())
			return;

		image->SetLayout(Layout::ImageLayout::TransferSrc, cmdList, 0, 0);
		VkBufferImageCopy copy{};
		copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copy.imageSubresource.layerCount = 1;
		copy.imageExtent = { flight.width, flight.height, 1 };
		vkCmdCopyImageToBuffer(cmdList->GetCommandBuffer(), *image->Get(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, flight.color->Get(), 1, &copy);
		image->SetLayout(Layout::ImageLayout::ShaderRead, cmdList, 0, 0);
		HostReadBarrier(cmdList);
		s.inFlight.push_back(std::move(flight));
	}

	void FrameCapture::Shutdown()
	{
		State &s = S();
		std::lock_guard lock(s.mutex);
		for (InFlight &flight : s.inFlight)
		{
			if (flight.color)
				flight.color->Destroy();
			if (flight.depth)
				flight.depth->Destroy();
		}
		s.inFlight.clear();
		s.queued.reset();
		s.queuedDump.reset();
	}
}

// -------------------------------------------------------
