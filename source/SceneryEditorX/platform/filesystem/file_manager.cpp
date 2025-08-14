/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* FileManager.cpp
* -------------------------------------------------------
* Created: 17/3/2025
* -------------------------------------------------------
*/
#include <commdlg.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
//#include <SceneryEditorX/asset/asset_manager.h>
#include <SceneryEditorX/core/time/time.h>
#include <SceneryEditorX/platform/config/editor_config.hpp>
#include <SceneryEditorX/platform/filesystem/file_manager.hpp>
#include <SceneryEditorX/scene/material.h>
#include <SceneryEditorX/scene/model_asset.h>
#include <SceneryEditorX/utils/string_utils.h>
#include <tiny_gltf.h>
#include <tiny_obj_loader.h>

/// -------------------------------------------------------

namespace SceneryEditorX::IO
{
    /// -------------------------------------------------------

    std::filesystem::path FileSystem::GetWorkingDir()
    {
        return std::filesystem::current_path();
    }

    void FileSystem::SetWorkingDir(const std::filesystem::path &path)
    {
        std::filesystem::current_path(path);
    }

    bool FileSystem::CreateDir(const std::filesystem::path &directory)
    {
        return std::filesystem::create_directories(directory);
    }

    bool FileSystem::CreateDir(const std::string &directory)
    {
        return CreateDir(std::filesystem::path(directory));
    }

    bool FileSystem::DirExists(const std::filesystem::path &directory)
    {
        return std::filesystem::exists(directory);
    }

    bool FileSystem::DirExists(const std::string &directory)
    {
        return DirExists(std::filesystem::path(directory));
    }

    bool FileSystem::DeleteFile(const std::filesystem::path &filepath)
    {
        if (!Exists(filepath))
            return false;

        if (std::filesystem::is_directory(filepath))
            return std::filesystem::remove_all(filepath) > 0;
        return std::filesystem::remove(filepath);
    }

    bool FileSystem::MoveFile(const std::filesystem::path &filepath, const std::filesystem::path &dest)
    {
        return Move(filepath, dest / filepath.filename());
    }

    bool FileSystem::CopyFile(const std::filesystem::path &filepath, const std::filesystem::path &dest)
    {
        return Copy(filepath, dest / filepath.filename());
    }

	bool FileSystem::Exists(const std::filesystem::path &filepath)
    {
        return std::filesystem::exists(filepath);
    }

    bool FileSystem::Exists(const std::string &filepath)
    {
        return std::filesystem::exists(std::filesystem::path(filepath));
    }

    bool FileSystem::Copy(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath)
    {
		if (Exists(newFilepath))
			return false;

		std::filesystem::copy(oldFilepath, newFilepath);
		return true;
    }

    bool FileSystem::Rename(const std::filesystem::path &oldFilepath, const std::filesystem::path &newFilepath)
    {
        return Move(oldFilepath, newFilepath);
    }

    bool FileSystem::RenameFilename(const std::filesystem::path &oldFilepath, const std::string &newName)
    {
        const std::filesystem::path newPath = oldFilepath.parent_path() / std::filesystem::path(newName + oldFilepath.extension().string());
        return Rename(oldFilepath, newPath);
    }

	bool FileSystem::IsDirectory(const std::filesystem::path &filepath)
    {
        return std::filesystem::is_directory(filepath);
    }

	FileStatus FileSystem::TryOpenFileAndWait(const std::filesystem::path &filepath, uint64_t waitms)
    {
        FileStatus fileStatus = TryOpenFile(filepath);
        if (fileStatus == FileStatus::Locked)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(operator""ms((unsigned long long)waitms));
            return TryOpenFile(filepath);
        }
        return fileStatus;
    }

	/// returns true <=> fileA was last modified more recently than fileB
    bool FileSystem::IsNewer(const std::filesystem::path &fileA, const std::filesystem::path &fileB)
    {
        return std::filesystem::last_write_time(fileA) > std::filesystem::last_write_time(fileB);
    }

    bool FileSystem::ShowFileInExplorer(const std::filesystem::path &path)
    {
        const auto absolutePath = std::filesystem::canonical(path);
        if (!Exists(absolutePath))
            return false;

    #ifdef SEDX_PLATFORM_WINDOWS
        std::string cmd = std::format("explorer.exe /select,\"{0}\"", absolutePath.string());
    #elif defined(SEDX_PLATFORM_LINUX)
        std::string cmd = std::format("xdg-open \"{0}\"", dirname(absolutePath.string().data()));
    #endif
        system(cmd.c_str());
        return true;
    }

    bool FileSystem::OpenDirectoryInExplorer(const std::filesystem::path &path)
    {
    #ifdef SEDX_PLATFORM_WINDOWS
        auto absolutePath = std::filesystem::canonical(path);
        if (!Exists(absolutePath))
            return false;

        ShellExecute(nullptr, reinterpret_cast<LPCSTR>(L"explore"), reinterpret_cast<LPCSTR>(absolutePath.c_str()), nullptr, nullptr, SW_SHOWNORMAL);
        return true;
    #elif defined(SEDX_PLATFORM_LINUX)
        return ShowFileInExplorer(path);
    #endif		
    }

    std::filesystem::path FileSystem::GetUniqueFileName(const std::filesystem::path &filepath)
    {
        if (!Exists(filepath))
            return filepath;

        int counter = 0;
        auto checkID = [&counter, filepath](const auto &checkID) -> std::filesystem::path {
            ++counter;
            const std::string counterStr = [&counter]
            {
                if (counter < 10)
                    return "0" + std::to_string(counter);

                return std::to_string(counter);
            }(); /// Pad with 0 if < 10;

            std::string newFileName = std::format("{} ({})", Utils::RemoveExtension(filepath.filename().string()), counterStr);

            if (filepath.has_extension())
                newFileName = std::format("{}{}", newFileName, filepath.extension().string());

            if (std::filesystem::exists(filepath.parent_path() / newFileName))
                return checkID(checkID);

            return filepath.parent_path() / newFileName;
        };

        return checkID(checkID);
    }


    /// -------------------------------------------------------

	/*
	std::string FileDialogs::OpenFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		CHAR currentDir[256] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		//ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)EditorApplication::Get().GetWindow().GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return {};

	}

	std::string FileDialogs::SaveFile(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = {};
		CHAR currentDir[256] = {};
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		//ofn.hwndOwner = glfwGetWin32Window((GLFWwindow*)EditorApplication::Get().GetWindow().GetNativeWindow());
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		if (GetCurrentDirectoryA(256, currentDir))
			ofn.lpstrInitialDir = currentDir;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		// Sets the default extension by extracting it from the filter
		ofn.lpstrDefExt = strchr(filter, '\0') + 1;

		if (GetSaveFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return {};
	}

    bool FileDialogs::IsTexture(const std::filesystem::path &path)
    {
        const std::string ext = path.extension().string();
        return
	        ext == ".jpg" ||
			ext == ".png" ||
			ext == ".jpeg" ||
			ext == ".tga" ||
			ext == ".bmp";
    }

    bool FileDialogs::IsModel(const std::filesystem::path &path)
    {
        const std::string ext = path.extension().string();
        return
	        ext == ".obj" ||
			ext == ".gltf" ||
			ext == ".glb" ||
			ext == ".fbx" ||
			ext == ".3ds";
    }

    std::vector<uint8_t> FileDialogs::ReadFileBytes(const std::filesystem::path &path)
    {
        std::ifstream input(path, std::ios::binary);
        std::vector<u8> bytes((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));
        input.close();
        return bytes;
    }

    void FileDialogs::WriteFileBytes(const std::filesystem::path &path, const std::vector<uint8_t> &content)
    {
        std::ofstream file(path, std::ofstream::binary);
        if (file.is_open())
        {
            file.write((char *)content.data(), content.size());
            file.close();
        }
    }

    void FileDialogs::WriteFile(const std::filesystem::path &path, const std::string &content)
    {
        std::ofstream file(path, std::ofstream::out);
        if (file.is_open())
        {
            file.write(content.data(), content.size());
            file.close();
        }
    }

    uint64_t FileDialogs::Import(const std::filesystem::path &path, AssetManager &assets)
    {
        Time::TimeLog t("AssetIO::Import(" + path.string() + ")");
        const std::string ext = path.extension().string();
        if (IsTexture(path))
        {
            return ImportTexture(path, assets);
        }
        else if (IsModel(path))
        {
            return ImportModel(path, assets);
        }
        return 0;
    }

    void FileDialogs::ReadTexture(const std::filesystem::path &path, std::vector<uint8_t> &data, int32_t &w, int32_t &h)
    {
        int32_t channels = 4;
        uint8_t *indata = stbi_load(path.string().c_str(), &w, &h, &channels, 4);
        data.resize(w * h * 4);
        memcpy(data.data(), indata, data.size());
        stbi_image_free(indata);
    }

    void FileDialogs::ImportTexture(const std::filesystem::path &path, Ref<TextureAsset> &t)
    {
        uint8_t *indata = stbi_load(path.string().c_str(), &t->width, &t->height, &t->channels, 4);
        t->data.resize(t->width * t->height * 4);
        memcpy(t->data.data(), indata, t->data.size());
        t->channels = 4;
        stbi_image_free(indata);
    }

    uint64_t FileDialogs::ImportTexture(const std::filesystem::path &path, AssetManager &assets)
    {
        auto t = assets.CreateAsset<TextureAsset>(path.stem().string());
        ImportTexture(path, t);
        return t->uuid;
    }

    uint64_t FileDialogs::ImportModel(const std::filesystem::path &path, const AssetManager &assets)
    {
        if (const std::string ext = path.extension().string(); ext == ".gltf" || ext == ".glb")
        {
            return ImportGLTF(path, assets);
        }
        else if (ext == ".fbx")
        {
            return ImportFBX(path, assets);
        }
        else if (ext == ".3ds")
        {
            return Import3DS(path, assets);
        }
        else if (ext == ".obj")
        {
            return ImportOBJ(path, assets);
        }
        return 0;
    }

    /**
	 * @brief Reads the raw bytes from a file.
	 * 
	 * This function opens a file in binary mode, reads its contents into a vector of bytes,
	 * and returns the vector. It uses the 'ate' flag to start reading at the end of the file
	 * to determine the file size, then reads the file from the beginning.
	 * 
	 * @param path The path of the file to read.
	 * @param filename The name of the file to read.
	 * @return std::vector<char> A vector containing the raw bytes of the file.
	*/

    std::vector<uint8_t> FileManager::ReadRawBytes(const std::filesystem::path &path)
	{
        std::ifstream input(path, std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator(input)), (std::istreambuf_iterator<char>()));
        input.close();
        return bytes;
	}

	/**
	 * @brief Reads the shader code from a file.
	 * 
	 * This function opens a file in binary mode, reads its contents into a vector of bytes,
	 * and returns the vector. It uses the 'ate' flag to start reading at the end of the file
	 * to determine the file size, then reads the file from the beginning.
	 * 
	 * @param filename The name of the shader file to read.
	 * @return std::vector<char> A vector containing the raw bytes of the shader file.
	*/
	std::vector<char> FileManager::ReadShaders(const std::string &filename)
	{
	    std::ifstream file(filename, std::ios::ate | std::ios::binary);
	
	    if (!file.is_open())
	    {
	        SEDX_CORE_ERROR("Failed to open file: {}", ToString(filename));
	        ErrMsg(std::string("Failed to open file: ") + ToString(filename));
	        return {}; /// Return empty vector on failure
	    }
	
	    size_t fileSize = file.tellg();
	    if (fileSize == 0)
	    {
	        SEDX_CORE_ERROR("File is empty: {}", ToString(filename));
	        return {};
	    }
	
	    std::vector<char> buffer(fileSize);
	
	    file.seekg(0);
	    file.read(buffer.data(), fileSize);
	
	    if (!file)
	    {
	        SEDX_CORE_ERROR("Failed to read entire file: {}", ToString(filename));
	        ErrMsg(std::string("Failed to read entire file: ") + ToString(filename));
	        return {};
	    }
	
	    file.close();
	
	    SEDX_CORE_INFO("Successfully read file: {} ({} bytes)", filename, fileSize);
	    return buffer;
	}
	
	/**
	 * @brief Reads the raw bytes from a file.
	 * 
	 * This function opens a file in binary mode, reads its contents into a vector of bytes,
	 * and returns the vector. It uses the 'ate' flag to start reading at the end of the file
	 * to determine the file size, then reads the file from the beginning.
	 * 
	 * @param filename The name of the file to read.
	 * @return std::vector<char> A vector containing the raw bytes of the file.
	*/
    std::vector<char> FileManager::ReadFile(const std::string &filename)
	{
	    std::ifstream file(filename, std::ios::ate | std::ios::binary);
	
	    if (!file.is_open())
	    {
	        SEDX_CORE_ERROR("Failed to open file: {}", ToString(filename));
	        ErrMsg(std::string("Failed to open file: ") + ToString(filename));
	        return {}; /// Return empty vector on failure
	    }
	
	    size_t fileSize = file.tellg();
	    if (fileSize == 0)
	    {
	        SEDX_CORE_ERROR("File is empty: {}", ToString(filename));
	        return {};
	    }
	
	    std::vector<char> buffer(fileSize);
	
	    file.seekg(0);
	    file.read(buffer.data(), fileSize);
	
	    if (!file)
	    {
	        SEDX_CORE_ERROR("Failed to read entire file: {}", ToString(filename));
	        ErrMsg(std::string("Failed to read entire file: ") + ToString(filename));
	        return {};
	    }
	
	    file.close();
	
	    SEDX_CORE_INFO("Successfully read file: {} ({} bytes)", filename, fileSize);
	    return buffer;
	}

    void FileManager::GetAppData()
    {
        if (const char *appDataPath = getenv("APPDATA"))
        {
            std::string appDataDir(appDataPath);
            appDataDir += "\\SceneryEditorX";
            std::filesystem::create_directories(appDataDir);
        }
        else
            SEDX_CORE_ERROR_TAG("FILE MANAGER", "Failed to get APPDATA environment variable");
    }

    /*
	uint64_t FileDialogs::ImportGLTF(const std::filesystem::path &path, AssetManager &manager)
    {
	    tinygltf::Model model;
	    tinygltf::TinyGLTF loader;
	    std::string err;
	    std::string warn;
	
	    bool ret = false;
	    if (path.extension() == ".gltf")
		{
	        ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
	    }
	    else
		{
	        ret = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
	    }
	
	    if (!warn.empty())
		{
	      SEDX_CORE_WARN("Warn: {}", warn.c_str());
	    }
	
	    if (!err.empty())
		{
	      SEDX_CORE_ERROR("Err: {}", err.c_str());
	    }
	
	    if (!ret)
		{
	      SEDX_CORE_ERROR("Failed to parse glTF");
	      return 0;
	    }
	
	    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
	    const auto getBuffer = [&](auto& accessor, auto& view) { return &model.buffers[view.buffer].data[view.byteOffset + accessor.byteOffset]; };

	    std::vector<Ref<TextureAsset::Texture>> loadedTextures(model.textures.size());
	    for (int i = 0; i < model.textures.size(); i++)
		{
	        tinygltf::Texture texture = model.textures[i];
	        tinygltf::Image image = model.images[texture.source];

	        std::vector<uint8_t> buffer(image.width * image.height * 4);
	        if (image.component == 3)
			{
	            unsigned char* rgba = buffer.data(); 
	            unsigned char* rgb = image.image.data();
	            for (uint32_t j = 0; j < image.width * image.height; j++)
				{
	                rgba[0] = rgb[0];
	                rgba[1] = rgb[1];
	                rgba[2] = rgb[2];
	                rgba += 4;
	                rgb += 3;
	            }
	        }
	        else
			{
	            memcpy(buffer.data(), image.image.data(), image.width * image.height * 4);
	        }

	        loadedTextures[i] = manager.CreateAsset<TextureAsset::Texture>(texture.name);
	        loadedTextures[i]->data = buffer;
	        loadedTextures[i]->width = image.width;
	        loadedTextures[i]->height = image.height;
	        loadedTextures[i]->channels = 4;
	    }
	
	    std::vector<Ref<MaterialAsset>> materials(model.materials.size());
	
	    for (int i = 0; i < materials.size(); i++)
		{
	        auto& mat = model.materials[i];
	        materials[i] = manager.CreateAsset<MaterialAsset>(mat.name);
	        // pbr
	        if (mat.values.contains("baseColorTexture"))
			{
	            materials[i]->colorMap = loadedTextures[mat.values["baseColorTexture"].TextureIndex()];
	        }
	        if (mat.values.contains("metallicRoughnessTexture"))
			{
	            materials[i]->metallicRoughnessMap = loadedTextures[mat.values["metallicRoughnessTexture"].TextureIndex()];
	        }
	        if (mat.values.contains("baseColorFactor"))
			{
	            materials[i]->color = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
	        }
	        if (mat.values.contains("roughnessFactor"))
			{
	            materials[i]->roughness = (float)mat.values["roughnessFactor"].Factor();
	        }
	        if (mat.values.contains("metallicFactor"))
			{
	            materials[i]->metallic = (float)mat.values["metallicFactor"].Factor();
	        }
	        // additional
	        if (mat.additionalValues.contains("normalTexture"))
			{
	            materials[i]->normalMap = loadedTextures[mat.additionalValues["normalTexture"].TextureIndex()];
	        }
	        if (mat.additionalValues.contains("emissiveTexture"))
			{
	            materials[i]->emissionMap = loadedTextures[mat.additionalValues["emissiveTexture"].TextureIndex()];
	        }
	        if (mat.additionalValues.contains("occlusionTexture"))
			{
	            materials[i]->aoMap = loadedTextures[mat.additionalValues["occlusionTexture"].TextureIndex()];
	        }
	        if (mat.additionalValues.contains("emissiveFactor"))
			{
	            materials[i]->emission = glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data());
	        }
	    }
	
	    std::vector<Ref<MeshAsset>> loadedMeshes;
	    std::vector<int> loadedMeshMaterials;
	    for (const tinygltf::Mesh& mesh : model.meshes)
		{
	        for (int i = 0; i < mesh.primitives.size(); i++)
			{
	            const tinygltf::Primitive& primitive = mesh.primitives[i];
	            std::string name = (!mesh.name.empty() ? mesh.name : path.stem().string()) + "_" + std::to_string(i);
	            Ref<MeshAsset>& desc = loadedMeshes.emplace_back(manager.CreateAsset<MeshAsset>(name));
	            loadedMeshMaterials.emplace_back(primitive.material);
	
	            float* bufferPos = nullptr;
	            float* bufferNormals = nullptr;
	            float* bufferTangents = nullptr;
	            float* bufferUV = nullptr;
	
	            int stridePos = 0;
	            int strideNormals = 0;
	            int strideTangents = 0;
	            int strideUV = 0;

                uint32_t vertexCount = 0;
                uint32_t indexCount = 0;
	
	            // position
	            {
	                auto it = primitive.attributes.find("POSITION");
	                SEDX_ASSERT(it != primitive.attributes.end(), "Primitive don't have position attribute");
	                const tinygltf::Accessor accessor = model.accessors[it->second];
	                const tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
	                bufferPos = reinterpret_cast<float *>(getBuffer(accessor, bufferView));
	                stridePos = accessor.ByteStride(bufferView) / sizeof(float);
	                vertexCount = accessor.count;
	            }
	
	            // normal
	            {
	                auto it = primitive.attributes.find("NORMAL");
	                if (it != primitive.attributes.end())
					{
	                    const tinygltf::Accessor accessor = model.accessors[it->second];
	                    const tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
	                    bufferNormals = (float*)getBuffer(accessor, bufferView);
	                    strideNormals = accessor.ByteStride(bufferView) / sizeof(float);
	                }
	            }
	
	            // tangent
	            {
	                auto it = primitive.attributes.find("TANGENT");
	                if (it != primitive.attributes.end())
					{
	                    const tinygltf::Accessor accessor = model.accessors[it->second];
	                    const tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
	                    bufferTangents = (float*)getBuffer(accessor, bufferView);
	                    strideTangents = accessor.ByteStride(bufferView) / sizeof(float);
	                }
	            }
	
	            // uvs
	            {
	                auto it = primitive.attributes.find("TEXCOORD_0");
	                if (it != primitive.attributes.end())
					{
	                    const tinygltf::Accessor accessor = model.accessors[it->second];
	                    const tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
	                    bufferUV = (float*)getBuffer(accessor, bufferView);
	                    strideUV = accessor.ByteStride(bufferView) / sizeof(float);
	                }
	            }
	
	            // vertices
	            for (uint32_t v = 0; v < vertexCount; v++)
				{
	                MeshVertex vertex{};
	                vertex.position = glm::make_vec3(&bufferPos[v * stridePos]);
	                vertex.normal = bufferNormals ? glm::make_vec3(&bufferNormals[v * strideNormals]) : Vec3(0);
	                vertex.texCoord = bufferUV ? glm::make_vec3(&bufferUV[v * strideUV]) : Vec3(0);
	                vertex.tangent = bufferTangents ? glm::make_vec4(&bufferTangents[v * strideTangents]) : Vec4(0);
	                desc->vertices.push_back(vertex);
	            }
	
	            // indices
	            SEDX_ASSERT(primitive.indices > -1, "Non indexed primitive not supported!");
	            {
	                const tinygltf::Accessor accessor = model.accessors[primitive.indices];
	                const tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
	                indexCount = accessor.count;
	                auto pushIndices = [&](auto* bufferIndex)
	                {
	                    for (u32 i = 0; i < indexCount; i++)
						{
	                        desc->indices.push_back(bufferIndex[i]);
	                    }
	                };
	                switch (accessor.componentType)
	                {
	                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
	                    pushIndices((uint32_t *)getBuffer(accessor, bufferView));
	                    break;
	                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
	                    pushIndices((uint16_t *)getBuffer(accessor, bufferView));
	                    break;
	                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
	                    pushIndices(getBuffer(accessor, bufferView));
	                    break;
	                default:
	                    SEDX_ASSERT(false, "Index type not supported!");
	                }
	            }
	
	            // calculate tangents
	            if (!bufferTangents)
				{
	                Vec3* tan1 = new Vec3[desc->vertices.size()*2];
	                Vec3* tan2 = tan1 + vertexCount;
	                for (int indexID = 0; indexID < desc->indices.size(); indexID += 3)
					{
	                    int index1 = desc->indices[indexID + 0];
	                    int index2 = desc->indices[indexID + 2];
	                    int index3 = desc->indices[indexID + 1];
	                    const auto& v1 = desc->vertices[index1];
	                    const auto& v2 = desc->vertices[index2];
	                    const auto& v3 = desc->vertices[index3];
	                    Vec3 e1 = v2.position - v1.position;
	                    Vec3 e2 = v3.position - v1.position;
	                    Vec2 duv1 = v2.texCoord - v1.texCoord;
	                    Vec2 duv2 = v3.texCoord - v1.texCoord;
	                    float f = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);
	                    Vec3 sdir = f * (duv2.y * e1 - duv1.y * e2);
	                    Vec3 tdir = f * (duv1.x * e2 - duv2.x * e1);
	                    tan1[index1] += sdir;
	                    tan1[index2] += sdir;
	                    tan1[index3] += sdir;
	                    tan2[index1] += tdir;
	                    tan2[index2] += tdir;
	                    tan2[index3] += tdir;
	                }
	                for (int a = 0; a < desc->vertices.size(); a++)
					{
	                    Vec3 t = tan1[a];
	                    auto& v = desc->vertices[a];
	                    Vec3 n = v.normal;
	                    v.tangent = Vec4(glm::normalize(t - n * glm::dot(t, n)), 1.0);
	                    v.tangent.w = (glm::dot(glm::cross(n, t), tan2[a]) < 0.0f) ? -1.0f : 1.0f;
	                }
	                delete[] tan1;
	            }
	        }
	    }
	
	    std::vector<Ref<Node>> loadedNodes;
	    for (const tinygltf::Node& node : model.nodes)
		{
	        Ref<Node> groupNode = manager.CreateObject<Node>(node.name);
	        if (node.mesh >= 0)
			{
	            Ref<MeshNode> meshNode = manager.CreateObject<MeshNode>(node.name);
	            meshNode->mesh = loadedMeshes[node.mesh];
	            int matId = loadedMeshMaterials[node.mesh];
	            if (matId >= 0) {
	                meshNode->material = materials[matId];
	            }
	            Node::SetParent(meshNode, groupNode);
	        }
	        if (node.camera >= 0)
			{
	            // todo: load camera
	        }
	        if (node.light >= 0)
			{
	            Ref<LightNode> lightNode = manager.CreateObject<LightNode>(node.name);
	            Node::SetParent(lightNode, groupNode);
	        }
	        if (node.translation.size() == 3)
			{
	            groupNode->position = { float(node.translation[0]), float(node.translation[1]), float(node.translation[2]) };
	        }
	        if (node.rotation.size() == 4)
			{
	            Quat quat = { float(node.rotation[3]), float(node.rotation[0]), float(node.rotation[1]), float(node.rotation[2]) };
	            groupNode->rotation = glm::degrees(glm::eulerAngles(quat));
	        }
	        if (node.scale.size() == 3)
			{
	            groupNode->scale = { float(node.scale[0]), float(node.scale[1]), float(node.scale[2]) };
	        }
	        if (node.matrix.size() == 16)
			{
	            glm::mat4 mat;
	            for (int i = 0; i < 16; i++)
				{
	                glm::value_ptr(mat)[i] = node.matrix[i];
	            }
	            Quat quat = {};
	            Vec3 skew;
	            Vec4 perspective;
	            glm::decompose(mat, groupNode->scale, quat, groupNode->position, skew, perspective);
	            groupNode->rotation = glm::degrees(glm::eulerAngles(quat));
	        }

	        loadedNodes.emplace_back(groupNode);
	    }
	    for (int i = 0; i >= model.nodes.size(); i++)
		{
	        for (auto child : model.nodes[i].children)
			{
	            Node::SetParent(loadedNodes[child], loadedNodes[i]);
	        }
	    }
	
	    std::vector<Ref<SceneAsset>> loadedScenes;
	    for (const tinygltf::Scene& scene : model.scenes)
		{
	        Ref<SceneAsset> s = loadedScenes.emplace_back(manager.CreateAsset<SceneAsset>(scene.name));
	        for (auto node : scene.nodes)
			{
	            s->Add(loadedNodes[node]);
	        }
	    }
	
	    if (loadedScenes.size()) return loadedScenes[0]->uuid;
        else return 0;
	}
	
	uint64_t FileDialogs::ImportOBJ(const std::filesystem::path &path, AssetManager &manager)
    {
	    SEDX_CORE_TRACE("Start loading mesh {}", path.string().c_str());
	    tinyobj::attrib_t attrib;
	    std::vector<tinyobj::shape_t> shapes;
	    std::vector<tinyobj::material_t> materials;
	    std::string err;
	    std::string warn;
	    std::string filename = path.stem().string();
	    std::string parentPath = path.parent_path().string() + "/";
	
	    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str(), parentPath.c_str(), true))
		{
	        SEDX_CORE_ERROR("{}", err);
	        SEDX_CORE_WARN("{}", warn);
	        SEDX_CORE_ERROR("Failed to load obj file {}", path.string().c_str());
	    }
	
	    if (!warn.empty())
		{
	        SEDX_CORE_WARN("Warning during load obj file {}: {}", path.string().c_str(), warn);
	    }
	
	    // convert obj material to my material
	    auto avg = [](const tinyobj::real_t value[3]) {return (value[0] + value[1] + value[2]) / 3.0f; };
	    std::vector<Ref<MaterialAsset>> materialAssets;
	    std::unordered_map<std::string, Ref<TextureAsset>> textureAssets;
	    for (auto &material : materials)
        {
	        Ref<MaterialAsset> asset = manager.CreateAsset<MaterialAsset>(filename + ":" + material.name);
	        //asset->color = Vec4(1, 0, 0, 1);
	        asset->color = Vec4(glm::make_vec3(material.diffuse), 1);
	        asset->emission = glm::make_vec3(material.emission);
	        asset->metallic = material.metallic;
	        if (material.specular != nullptr)
			{
	            asset->roughness = 1.0f - avg(material.specular);
	        }
	        else
			{
	            asset->roughness = material.roughness;
	        }
	        if (!material.diffuse_texname.empty())
			{
	            if (!textureAssets.contains(material.diffuse_texname))
				{
	                asset->colorMap = manager.CreateAsset<TextureAsset>(material.diffuse_texname);
	                ImportTexture(parentPath + material.diffuse_texname, asset->colorMap);
	                textureAssets[material.diffuse_texname] = asset->colorMap;
	            }
	            else
				{
	                asset->colorMap = textureAssets[material.diffuse_texname];
	            }
	        }
	        if (!material.normal_texname.empty())
			{
	            if (!textureAssets.contains(material.normal_texname))
				{
	                asset->normalMap = manager.CreateAsset<TextureAsset>(material.normal_texname);
	                ImportTexture(parentPath + material.normal_texname, asset->normalMap);
	                textureAssets[material.normal_texname] = asset->normalMap;
	            }
	            else
				{
	                asset->normalMap = textureAssets[material.normal_texname];
	            }
	        }
	        materialAssets.push_back(asset);
	    }
	
	    Ref<SceneAsset> scene = manager.CreateAsset<SceneAsset>(filename);
        const Ref<Node> parentNode = AssetManager::CreateObject<Node>(filename);
	    scene->Add(parentNode);
	    for (auto &shape : shapes)
        {
	        if (shape.mesh.indices.empty())
			{
	            continue;
	        }
	        std::unordered_map<MeshVertex, uint32_t> uniqueVertices{};
            size_t j = 0;
	        size_t lastMaterialId = !shape.mesh.material_ids.empty() ? shape.mesh.material_ids[0] : -1;
	        Ref<MeshAsset> asset = manager.CreateAsset<MeshAsset>(filename + ":" + shape.name);
	        for (const auto&[vertex_index, normal_index, texcoord_index] : shape.mesh.indices)
			{
	            MeshVertex vertex{};
	
	            vertex.position = {
	                attrib.vertices[3 * vertex_index + 0],
	                attrib.vertices[3 * vertex_index + 1],
	                attrib.vertices[3 * vertex_index + 2]
	            };
	
	            if (normal_index != -1)
				{
	                vertex.normal = {
	                    attrib.normals[3 * normal_index + 0],
	                    attrib.normals[3 * normal_index + 1],
	                    attrib.normals[3 * normal_index + 2],
	                };
	            }
	
	            if (texcoord_index == -1)
				{
	                vertex.texCoord = { 0, 0 };
	            }
	            else
				{
	                vertex.texCoord = {
	                    attrib.texcoords[2 * texcoord_index + 0],
	                    1.0f - attrib.texcoords[2 * texcoord_index + 1]
	                };
	            }
	
	            if (!uniqueVertices.contains(vertex))
				{
	                uniqueVertices[vertex] = static_cast<uint32_t>(asset->vertices.size());
	                asset->vertices.push_back(vertex);
	            }
	
	            asset->indices.push_back(uniqueVertices[vertex]);
	            j += 1;
	
	            if (j % 3 == 0)
				{
                    if (size_t faceId = j / 3; faceId >= shape.mesh.material_ids.size() || std::cmp_not_equal(shape.mesh.material_ids[faceId], lastMaterialId))
					{
                        int splittedShapeIndex = 0;
                        asset->name += "_" + std::to_string(splittedShapeIndex);
	                    Ref<MeshNode> model = AssetManager::CreateObject<MeshNode>(asset->name);
	                    Node::SetParent(model, parentNode);
	                    model->mesh = asset;
	                    if (std::cmp_not_equal(lastMaterialId, -1))
						{
	                        model->material = materialAssets[lastMaterialId];
	                    }
	                    if (faceId < shape.mesh.material_ids.size())
						{
	                        lastMaterialId = shape.mesh.material_ids[faceId];
	                    }
	                    uniqueVertices.clear();
	                }
	            }
	        }
	    }

	    SEDX_CORE_INFO("Objects: %d", parentNode->children.size());
	    return scene->uuid;
	}
	*/

    /// -------------------------------------------------------


} // namespace SceneryEditorX::IO

/// -------------------------------------------------------
