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
<<<<<<<< HEAD:source/SceneryEditorX/filesystem/file_manager.hpp
 * FileManager.hpp
 * -------------------------------------------------------
 * Created: 17/3/2025
 * -------------------------------------------------------
 */
#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <SceneryEditorX/core/memory/buffer.h>
========
 * properties.h
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/renderer/ui/ui_widget.h>
>>>>>>>> e8126a5968df90e6d0729cf1257081d7862db7d6:source/Editor/panels/properties.h

// -------------------------------------------------------

namespace SceneryEditorX::IO
{
	class EditorLayer;
	class Renderable;
	class Script;
	class Light;
	class Camera;
	class Terrain;
	class Spline;
	class Volume;
	class ParticleSystem;
	class MaterialAsset;
	class Entity;
	class Editor;

	/**
<<<<<<<< HEAD:source/SceneryEditorX/filesystem/file_manager.hpp
	 * @class FileManager
	 * @brief Manages file operations such as reading, writing, and querying file status.
	 */
	class FileManager
	{
	public:
		/**
		 * @brief Opens the specified file and reads its contents into a vector of bytes.
		 * @param path The path to the file to read.
		 * @return A vector containing the raw bytes read from the file.
		 */
		static std::vector<uint8_t> ReadRawBytes(const std::filesystem::path &path);

		/**
		 * @brief Opens the specified shader file and reads its contents into a vector of bytes.
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
		 * @param path 
		 * @return 
		 */
		static bool IsTexture(const std::filesystem::path &path);

		/**
		 * @brief 
		 * @param path 
		 * @return 
		 */
		static bool IsModel(const std::filesystem::path &path);

		/**
		 * @brief  
		 * @param path 
		 * @return 
		 */
		static bool IsSceneFile(const std::string &path);

========
	 * @class Properties
	 * @brief The Properties widget displays detailed information and editable fields for the
	 * currently selected entity or resource in the editor. 
	 * It allows users to inspect and modify components, materials, and other properties of scene objects. 
	 * The widget dynamically updates its contents based on the type of object being inspected, 
	 * providing context-sensitive controls for each component type.
	 */
	class Properties : public Widget
	{
	public:
		Properties(EditorLayer *editor);
	
		void OnTickVisible() override;
	
		static void Inspect(Entity *entity);
		static void Inspect(const Ref<MaterialAsset> &material);
	
		// inspected resources
		static WeakRef<MaterialAsset> m_inspected_material;
	
	private:
		void ShowEntity(Entity* transform) const;
		void ShowScript(Script* script) const;
		void ShowLight(Light* light) const;
		void ShowRender(Renderable* renderable) const;
		void ShowMaterial(MaterialAsset* material) const;
		void ShowCamera(Camera* camera) const;
		void ShowTerrain(Terrain* terrain) const;
		void ShowSpline(Spline* spline) const;
		void ShowVolume(Volume* volume) const;
		void ShowParticleSystem(ParticleSystem* particle_system) const;
	
		static void ShowAddComponentButton();
		static void ComponentContextMenu_Add();
		static void ShowSaveAsPrefabPopup(Entity* entity);
>>>>>>>> e8126a5968df90e6d0729cf1257081d7862db7d6:source/Editor/panels/properties.h
	};
	
}

// -------------------------------------------------------
