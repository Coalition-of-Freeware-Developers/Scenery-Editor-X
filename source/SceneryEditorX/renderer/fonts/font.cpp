/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* font.cpp
* -------------------------------------------------------
* Created: 12/7/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/asset/asset_manager.h>
#include <SceneryEditorX/platform/file_manager.hpp>
#include <SceneryEditorX/renderer/fonts/font.h>
#include <SceneryEditorX/asset/ecs.h>
#include <SceneryEditorX/utils/pointers.h>
#include <SceneryEditorX/core/memory/buffer.h>
#include <msdf-atlas-gen/msdf-atlas-gen/msdf-atlas-gen.h>
#include <msdf-atlas-gen/msdf-atlas-gen/TightAtlasPacker.h>
#include <utility>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	using namespace msdf_atlas;

    /// -------------------------------------------------------

	struct FontInput
    {
		Memory::Buffer fontData;
		GlyphIdentifierType glyphIdentifierType;
		const char* charsetFilename;
		double fontScale;
		const char* fontName;
	};

    /// -------------------------------------------------------

	struct Configuration
    {
		ImageType imageType;
		msdf_atlas::ImageFormat imageFormat;
		YDirection yDirection;
		int width, height;
		double emSize;
		double pxRange;
		double angleThreshold;
		double miterLimit;
		void (*edgeColoring)(msdfgen::Shape&, double, unsigned long long);
		bool expensiveColoring;
		unsigned long long coloringSeed;
		GeneratorAttributes generatorAttributes;
	};

    /// -------------------------------------------------------

	#define DEFAULT_ANGLE_THRESHOLD 3.0
	#define DEFAULT_MITER_LIMIT 1.0
	#define LCG_MULTIPLIER 6364136223846793005ull
	#define LCG_INCREMENT 1442695040888963407ull
	#define THREADS 8

    /// -------------------------------------------------------

	namespace Utils
    {

		static std::filesystem::path GetCacheDirectory()
		{
			//return Project::GetCacheDirectory() / "FontAtlases";
			return "cache/fontAtlases";
		}

		static void CreateCacheDirectoryIfNeeded()
		{
            if (const std::filesystem::path cacheDirectory = GetCacheDirectory(); !std::filesystem::exists(cacheDirectory))
				std::filesystem::create_directories(cacheDirectory);
		}
	}

    /// -------------------------------------------------------

	struct AtlasHeader
	{
		uint32_t Type = 0;
		uint32_t Width = 0, Height = 0;
	};

    /// -------------------------------------------------------

	static bool TryReadFontAtlasFromCache(const std::string& fontName, float fontSize, AtlasHeader& header, void*& pixels, Memory::Buffer& storageBuffer)
	{
		const std::string filename = std::format("{0}-{1}.fCache", fontName, fontSize);

        if (const std::filesystem::path filepath = Utils::GetCacheDirectory() / filename; std::filesystem::exists(filepath))
		{
			storageBuffer = IO::FileSystem::ReadBytes(filepath);
			header = *storageBuffer.As<AtlasHeader>();
			pixels = (uint8_t*)storageBuffer.data + sizeof(AtlasHeader);
			return true;
		}
		return false;
	}

    /// -------------------------------------------------------

	static void CacheFontAtlas(const std::string& fontName, float fontSize, AtlasHeader header, const void* pixels)
	{
		Utils::CreateCacheDirectoryIfNeeded();
		const std::string filename = std::format("{0}-{1}.fCache", fontName, fontSize);
		const std::filesystem::path filepath = Utils::GetCacheDirectory() / filename;

		std::ofstream stream(filepath, std::ios::binary | std::ios::trunc);
		if (!stream)
		{
			stream.close();
			SEDX_CORE_ERROR_TAG("Renderer", "Failed to cache font atlas to {0}", filepath.string());
			return;
		}

		stream.write((char*)&header, sizeof(AtlasHeader));
		stream.write((char*)pixels, header.Width * header.Height * sizeof(float) * 4);
	}

    /// -------------------------------------------------------

	template <typename T, typename S, int N, GeneratorFunction<S, N> GEN_FN>
	static Ref<Texture2D> CreateAndCacheAtlas(const std::string& fontName, float fontSize, const std::vector<GlyphGeometry>& glyphs, const FontGeometry& fontGeometry, const Configuration& config)
	{
		ImmediateAtlasGenerator<S, N, GEN_FN, BitmapAtlasStorage<T, N>> generator(config.width, config.height);
		generator.setAttributes(config.generatorAttributes);
		generator.setThreadCount(THREADS);
		generator.generate(glyphs.data(), (int)glyphs.size());

		msdfgen::BitmapConstRef<T, N> bitmap = (msdfgen::BitmapConstRef<T, N>) generator.atlasStorage();

		AtlasHeader header;
		header.Width = bitmap.width;
		header.Height = bitmap.height;
		CacheFontAtlas(fontName, fontSize, header, bitmap.pixels);

		TextureSpecification spec;
        spec.Format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		spec.Width = header.Width;
		spec.Height = header.Height;
		spec.GenerateMips = false;
		spec.SamplerWrap = UVWrap::Clamp;
		spec.DebugName = "FontAtlas";
		Ref<Texture2D> texture = Texture2D::Create(spec, bitmap.pixels);
		return texture;
	}

    /// -------------------------------------------------------

	static Ref<Texture2D> CreateCachedAtlas(AtlasHeader header, const void* pixels)
	{
		TextureSpecification spec;
        spec.Format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
		spec.Width = header.Width;
		spec.Height = header.Height;
		spec.GenerateMips = false;
		spec.SamplerWrap = UVWrap::Clamp;
		spec.DebugName = "FontAtlas";
        Ref<Texture2D> texture = CreateRef<Texture2D>(spec, pixels);
		return texture;
	}

    /// -------------------------------------------------------

	Font::Font(const std::filesystem::path& filepath) : m_MSDFData(new MSDFData())
	{
		m_Name = filepath.stem().string();

		Memory::Buffer buffer = IO::FileSystem::ReadBytes(filepath);
		CreateAtlas(buffer);
		buffer.Release();
	}

    /// -------------------------------------------------------

	Font::Font(std::string name, Memory::Buffer buffer) : m_Name(std::move(name)), m_MSDFData(new MSDFData()), buffer(buffer)
	{
		CreateAtlas(buffer);
	}

    /// -------------------------------------------------------

	Font::~Font()
	{
		delete m_MSDFData;
	}

    /// -------------------------------------------------------

	void Font::CreateAtlas(Memory::Buffer buffer)
	{
		FontInput fontInput = { };
		Configuration config = { };
		fontInput.fontData = buffer;
		fontInput.glyphIdentifierType = GlyphIdentifierType::UNICODE_CODEPOINT;
		fontInput.fontScale = -1;
		config.imageType = ImageType::MSDF;
		config.imageFormat = msdf_atlas::ImageFormat::BINARY_FLOAT;
		config.yDirection = YDirection::BOTTOM_UP;
		config.edgeColoring = msdfgen::edgeColoringInkTrap;
		const char* imageFormatName = nullptr;
		int fixedWidth = -1, fixedHeight = -1;
		config.generatorAttributes.config.overlapSupport = true;
		config.generatorAttributes.scanlinePass = true;
		double minEmSize = 0;
		double rangeValue = 2.0;
		DimensionsConstraint atlasSizeConstraint = DimensionsConstraint::MULTIPLE_OF_FOUR_SQUARE;
		config.angleThreshold = DEFAULT_ANGLE_THRESHOLD;
		config.miterLimit = DEFAULT_MITER_LIMIT;
		config.imageType = ImageType::MTSDF;
		config.emSize = 40;

		///< Load fonts
		bool anyCodepointsAvailable = false;
		class FontHolder
		{
			msdfgen::FreetypeHandle* ft;
			msdfgen::FontHandle* font;
		public:
			FontHolder() : ft(msdfgen::initializeFreetype()), font(nullptr) {}
			~FontHolder()
			{
				if (ft)
				{
					if (font)
                        msdfgen::destroyFont(font);

                    msdfgen::deinitializeFreetype(ft);
				}
			}

			bool load(const Memory::Buffer &buffer)
			{
				if (ft && buffer)
				{
					if (font)
						msdfgen::destroyFont(font);
					if ((font = msdfgen::loadFontData(ft, buffer.As<const msdfgen::byte>(), static_cast<int>(buffer.size))))
						return true;
				}
				return false;
			}

            explicit operator msdfgen::FontHandle* () const { return font; }

		} font;

		bool success = font.load(fontInput.fontData);
		SEDX_CORE_ASSERT(success);

		if (fontInput.fontScale <= 0)
			fontInput.fontScale = 1;

		///< Load character set
		fontInput.glyphIdentifierType = GlyphIdentifierType::UNICODE_CODEPOINT;
		Charset charset;

		///< From ImGui
		static const uint32_t charsetRanges[] =
		{
			0x0020, 0x00FF, ///< Basic Latin + Latin Supplement
			0x0400, 0x052F, ///< Cyrillic + Cyrillic Supplement
			0x2DE0, 0x2DFF, ///< Cyrillic Extended-A
			0xA640, 0xA69F, ///< Cyrillic Extended-B
			0,
		};

		for (int range = 0; range < 8; range += 2)
		{
			for (uint32_t c = charsetRanges[range]; c <= charsetRanges[range + 1]; c++)
				charset.add(c);
		}

		///< Load glyphs
		m_MSDFData->FontGeometry = FontGeometry(&m_MSDFData->Glyphs);
		int glyphsLoaded = -1;
		switch (fontInput.glyphIdentifierType)
		{
			case GlyphIdentifierType::GLYPH_INDEX:
				glyphsLoaded = m_MSDFData->FontGeometry.loadGlyphset(static_cast<msdfgen::FontHandle *>(font), fontInput.fontScale, charset);
				break;
			case GlyphIdentifierType::UNICODE_CODEPOINT:
				glyphsLoaded = m_MSDFData->FontGeometry.loadCharset(static_cast<msdfgen::FontHandle *>(font), fontInput.fontScale, charset);
				anyCodepointsAvailable |= glyphsLoaded > 0;
				break;
		}

		SEDX_CORE_ASSERT(glyphsLoaded >= 0);
		SEDX_CORE_TRACE_TAG("Renderer", "Loaded geometry of {} out of {} glyphs", glyphsLoaded, (int)charset.size());
		///< List missing glyphs
		if (std::cmp_less(glyphsLoaded, charset.size()))
            SEDX_CORE_WARN_TAG("Renderer", "Font {} is missing {} {}", m_Name, (int)charset.size() - glyphsLoaded, fontInput.glyphIdentifierType == GlyphIdentifierType::UNICODE_CODEPOINT ? "codepoints" : "glyphs");

        if (fontInput.fontName)
			m_MSDFData->FontGeometry.setName(fontInput.fontName);

		/**
		 * NOTE: we still need to "pack" the font to determine atlas metadata, though this could also be cached.
		 * The most intensive part is the actual atlas generation, which is what we do cache - it takes
		 * around 96% of total time spent in this Font constructor.
		 */
	    ///< Determine final atlas dimensions, scale and range, pack glyphs
		double pxRange = rangeValue;
		bool fixedDimensions = fixedWidth >= 0 && fixedHeight >= 0;
		bool fixedScale = config.emSize > 0;
		TightAtlasPacker atlasPacker;
		if (fixedDimensions)
			atlasPacker.setDimensions(fixedWidth, fixedHeight);
		else
			atlasPacker.setDimensionsConstraint(atlasSizeConstraint);
		atlasPacker.setSpacing(0);
		if (fixedScale)
			atlasPacker.setScale(config.emSize);
		else
			atlasPacker.setMinimumScale(minEmSize);
		atlasPacker.setPixelRange(msdfgen::Range(pxRange));
		atlasPacker.setMiterLimit(config.miterLimit);
		if (int remaining = atlasPacker.pack(m_MSDFData->Glyphs.data(), (int)m_MSDFData->Glyphs.size()))
		{
			if (remaining < 0)
			{
				SEDX_CORE_ASSERT(false);
			}
			else
			{
				SEDX_CORE_ERROR_TAG("Renderer", "Error: Could not fit {0} out of {1} glyphs into the atlas.", remaining, (int)m_MSDFData->Glyphs.size());
				SEDX_CORE_ASSERT(false);
			}
		}
		atlasPacker.getDimensions(config.width, config.height);
		SEDX_CORE_ASSERT(config.width > 0 && config.height > 0);
		config.emSize = atlasPacker.getScale();
		config.pxRange = atlasPacker.getPixelRange().lower; // Use .lower for double
		if (!fixedScale)
			SEDX_CORE_TRACE_TAG("Renderer", "Glyph size: {0} pixels/EM", config.emSize);
		if (!fixedDimensions)
			SEDX_CORE_TRACE_TAG("Renderer", "Atlas dimensions: {0} x {1}", config.width, config.height);


		///< Edge coloring
		if (config.imageType == ImageType::MSDF || config.imageType == ImageType::MTSDF)
		{
			if (config.expensiveColoring)
			{
				Workload([&glyphs = m_MSDFData->Glyphs, &config](int i, int threadNo) -> bool
				{
					unsigned long long glyphSeed = (LCG_MULTIPLIER * (config.coloringSeed ^ i) + LCG_INCREMENT) * !!config.coloringSeed;
					glyphs[i].edgeColoring(config.edgeColoring, config.angleThreshold, glyphSeed);
					return true;
				}, (int)m_MSDFData->Glyphs.size()).finish(THREADS);
			}
			else
			{
				unsigned long long glyphSeed = config.coloringSeed;
				for (GlyphGeometry& glyph : m_MSDFData->Glyphs)
				{
					glyphSeed *= LCG_MULTIPLIER;
					glyph.edgeColoring(config.edgeColoring, config.angleThreshold, glyphSeed);
				}
			}
		}

		/// Check cache here
		Memory::Buffer storageBuffer;
        void* pixels;
		if (AtlasHeader header; TryReadFontAtlasFromCache(m_Name, static_cast<float>(config.emSize), header, pixels, storageBuffer))
		{
			m_TextureAtlas = CreateCachedAtlas(header, pixels);
			storageBuffer.Release();
		}
		else
		{
			bool floatingPointFormat = true;
			Ref<Texture2D> texture;
			switch (config.imageType)
			{
				case ImageType::MSDF:
					if (floatingPointFormat)
						texture = CreateAndCacheAtlas<float, float, 3, msdfGenerator>(m_Name, static_cast<float>(config.emSize), m_MSDFData->Glyphs, m_MSDFData->FontGeometry, config);
					else
						texture = CreateAndCacheAtlas<byte, float, 3, msdfGenerator>(m_Name, static_cast<float>(config.emSize), m_MSDFData->Glyphs, m_MSDFData->FontGeometry, config);
					break;
				case ImageType::MTSDF:
					if (floatingPointFormat)
						texture = CreateAndCacheAtlas<float, float, 4, mtsdfGenerator>(m_Name, static_cast<float>(config.emSize), m_MSDFData->Glyphs, m_MSDFData->FontGeometry, config);
					else
						texture = CreateAndCacheAtlas<byte, float, 4, mtsdfGenerator>(m_Name, static_cast<float>(config.emSize), m_MSDFData->Glyphs, m_MSDFData->FontGeometry, config);
					break;
                case ImageType::HARD_MASK:
                    break;
                case ImageType::SOFT_MASK:
                    break;
                case ImageType::SDF:
                    break;
                case ImageType::PSDF:
                    break;
                default: ;
            }

			m_TextureAtlas = texture;
		}
	}

    /// -------------------------------------------------------

	void Font::Init()
	{
		s_DefaultFont = CreateRef<Font>("assets/fonts/opensans/OpenSans-Regular.ttf");
		s_DefaultMonoSpacedFont = CreateRef<Font>("assets/fonts/roboto.ttf");
	}

    /// -------------------------------------------------------

	void Font::Shutdown()
	{
		s_DefaultMonoSpacedFont.Reset();
		s_DefaultFont.Reset();
	}

    /// -------------------------------------------------------

	Ref<Font> Font::GetDefaultFont()
	{
		return s_DefaultFont;
	}

	Ref<Font> Font::GetDefaultMonoSpacedFont()
	{
		return s_DefaultMonoSpacedFont;
	}

	Ref<Font> Font::GetFontAssetForTextComponent(const TextComponent& textComponent)
	{
        const UUID32 handle = textComponent.FontHandle;
		if (handle == s_DefaultFont->Handle || !AssetManager::GetAsset<Font>(static_cast<uint32_t>(handle)))
			return s_DefaultFont;
		
		return AssetManager::GetAsset<Font>(static_cast<uint32_t>(handle));
	}

    /// -------------------------------------------------------

}

/// -------------------------------------------------------
