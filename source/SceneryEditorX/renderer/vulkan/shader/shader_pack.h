/**
 * -------------------------------------------------------
 * RailNex Train Simulator
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 RailNex
 * -------------------------------------------------------
 * shader_pack.h
 * -------------------------------------------------------
 * rnxForge.dll
 * -------------------------------------------------------
 * Created: 22/06/2026
 * -------------------------------------------------------
 */
#pragma once
#include "shader.h"
#include "shader_packfile.h"
#include <filesystem>

// -------------------------------------------------------

namespace SceneryEditorX 
{
	class ShaderLibrary;

	/**
	 * @class ShaderPack 
	 * @brief Represents a shader pack containing multiple shaders.
	 */
	class ShaderPack : public RefCounted
	{
	public:
		ShaderPack() = default;
		ShaderPack(const std::filesystem::path& path);

		bool IsLoaded() const { return m_Loaded; }
		bool Contains(std::string_view name) const;

		/**
		 * @brief Loads a shader from the shader pack.
		 * @param name The name of the shader to load.
		 * @return A reference to the loaded shader.
		 */
		Ref<Shader> LoadShader(std::string_view name);

		/**
		 * @brief Creates a shader pack from a shader library.
		 * @param shaderLibrary The shader library to create the pack from.
		 * @param path The path to save the shader pack.
		 * @return A reference to the created shader pack.
		 */
		static Ref<ShaderPack> CreateFromLibrary(Ref<ShaderLibrary> shaderLibrary, const std::filesystem::path& path);

	private:
		bool m_Loaded = false;
		ShaderPackFile m_File;
		std::filesystem::path m_Path;
	};
}

// -------------------------------------------------------
