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
 * properties.h
 * -------------------------------------------------------
 * Created: 18/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/renderer/ui/editor_panel.h>

// -------------------------------------------------------

namespace SceneryEditorX
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
	 * @class Properties
	 * @brief The Properties widget displays detailed information and editable fields for the
	 * currently selected entity or resource in the editor. 
	 * It allows users to inspect and modify components, materials, and other properties of scene objects. 
	 * The widget dynamically updates its contents based on the type of object being inspected, 
	 * providing context-sensitive controls for each component type.
	 */
	class Properties : public UI::EditorPanel
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
	};
	
}

// -------------------------------------------------------
