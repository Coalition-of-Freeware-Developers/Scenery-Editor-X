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
 * xp_weather_art.cpp
 * -------------------------------------------------------
 * Created: 25/10/2026
 * -------------------------------------------------------
 */
#include "xp_weather_art.h"
#include "xp_library_manager.h"
#include "xp_texture_loader.h"
#include <array>
#include <chrono>
#include <future>
#include <SceneryEditorX/filesystem/vfs.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <SceneryEditorX/renderer/renderer.h>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	namespace fs = std::filesystem;

	namespace
	{
		constexpr const char *LOG_TAG = "XPlaneWeather";

		struct ArtFile
		{
			const char *name;
			TextureUsage usage;
		};

		// In Renderer::WeatherArt's order
		constexpr std::array<ArtFile, 7> ART_FILES = { {
			{ "snow_ALB.png", TextureUsage::Albedo },
			{ "snow_NML.png", TextureUsage::Normal },
			{ "snow_DCL.png", TextureUsage::Data },
			{ "ice_ALB.png", TextureUsage::Albedo },
			{ "ice_NML.png", TextureUsage::Normal },
			{ "ice_DCL.png", TextureUsage::Data },
			{ "noise.png", TextureUsage::Data },
		} };

		using DecodedArt = std::array<DecodedTexture, ART_FILES.size()>;

		struct State
		{
			fs::path install;					// The install the art was (or is being) loaded from
			std::future<DecodedArt> pending;
		};
		State s_State;

		DecodedArt DecodeArt(const fs::path &folder)
		{
			DecodedArt decoded;
			for (size_t i = 0; i < ART_FILES.size(); ++i)
				decoded[i] = TextureLoader::Decode(TextureLoader::ResolveTexturePath(folder, ART_FILES[i].name), ART_FILES[i].usage);
			return decoded;
		}
	}

	void WeatherArt::Update()
	{
		const fs::path install = LibraryManager::GetInstallPath();
		if (!install.empty() && install != s_State.install && !s_State.pending.valid())
		{
			s_State.install = install;
			const fs::path folder = install / "Resources" / "bitmaps" / "world" / "weather";
			s_State.pending = std::async(std::launch::async, DecodeArt, folder);
		}

		if (!s_State.pending.valid() || s_State.pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
			return;

		DecodedArt decoded = s_State.pending.get();
		std::array<Ref<ImageResource>, ART_FILES.size()> images;
		for (size_t i = 0; i < ART_FILES.size(); ++i)
		{
			TextureInfo info;
			images[i] = TextureLoader::Upload(decoded[i], &info);
			if (!images[i])
			{
				SEDX_CORE_WARN_TAG(LOG_TAG, "Weather art missing or unreadable: {}{} (snow and ice draw plain)", ART_FILES[i].name,
					info.error.empty() ? std::string() : " (" + info.error + ")");
				return;
			}
		}

		Renderer::WeatherArt art;
		art.snowAlbedo = images[0];
		art.snowNormal = images[1];
		art.snowDecal = images[2];
		art.iceAlbedo = images[3];
		art.iceNormal = images[4];
		art.iceDecal = images[5];
		art.noise = images[6];
		Renderer::SetWeatherArt(art);
		SEDX_CORE_INFO_TAG(LOG_TAG, "Weather art loaded from {}", VFS::ToUtf8(s_State.install / "Resources" / "bitmaps" / "world" / "weather"));
	}
}

// -------------------------------------------------------
