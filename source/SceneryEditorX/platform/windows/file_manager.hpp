/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* FileManager.hpp
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/

#pragma once

#include <SceneryEditorX/scene/asset_manager.h>
#include <string>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::IO
{

	class FileManager
	{
	public:
		/**
		 * @brief Reads raw bytes from a file.
		 * 
		 * This function opens the specified file and reads its contents into a vector of bytes.
		 * 
		 * @param path
		 * @return A vector containing the raw bytes read from the file.
         */
        static std::vector<uint8_t> ReadRawBytes(const std::filesystem::path &path);

        /**
		 * @brief Reads shader code from a file.
		 * 
		 * This function opens the specified shader file and reads its contents into a vector of bytes.
		 * 
		 * @param filename The name of the shader file to read.
		 * @return A vector containing the raw bytes read from the shader file.
         */
        static std::vector<char> ReadShaders(const std::string &filename);

        /**
         * @brief Reads the raw bytes from a file.
         * @param filename The name of the file to read.
         * @return std::vector<char> A vector containing the raw bytes of the file.
         */
        static std::vector<char> ReadFile(const std::string &filename);

        /**
         * @brief
         */
        static void GetAppData();


	};

    // -------------------------------------------------------

	class FileDialogs
	{
	public:
		/// These return empty strings if cancelled
		GLOBAL std::string OpenFile(const char* filter);
		GLOBAL std::string SaveFile(const char* filter);

		// -------------------------------------------------------

        LOCAL bool IsTexture(const std::filesystem::path &path);
        LOCAL bool IsModel(const std::filesystem::path &path);

        LOCAL std::vector<uint8_t> ReadFileBytes(const std::filesystem::path &path);
        LOCAL void WriteFileBytes(const std::filesystem::path &path, const std::vector<uint8_t> &content);
        GLOBAL void WriteFile(const std::filesystem::path &path, const std::string &content);

		// -------------------------------------------------------
		/*
        uint64_t Import(const std::filesystem::path &path, AssetManager &assets);
        uint64_t ImportModel(const std::filesystem::path &path, const AssetManager &assets);

		uint64_t ImportGLTF(const std::filesystem::path& path, AssetManager& manager);
        uint64_t ImportOBJ(const std::filesystem::path& path, AssetManager& manager);
        uint64_t ImportFBX(const std::filesystem::path &path, const AssetManager &assets);
        uint64_t Import3DS(const std::filesystem::path &path, const AssetManager &assets);
        */
		// -------------------------------------------------------

        GLOBAL void ImportTexture(const std::filesystem::path &path, Ref<TextureAsset> &t);
        //LOCAL uint64_t ImportTexture(const std::filesystem::path &path, AssetManager &assets);
        INTERNAL void ReadTexture(const std::filesystem::path &path, std::vector<uint8_t> &data, int32_t &w, int32_t &h);

	};

	
} // namespace SceneryEditorX::IO

// -------------------------------------------------------
