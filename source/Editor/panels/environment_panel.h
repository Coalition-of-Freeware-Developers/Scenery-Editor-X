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
 * environment_panel.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <Editor/core/editor_context.h>
#include <SceneryEditorX/environment/environment_state.h>
#include <SceneryEditorX/renderer/ui/editor_panel.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class EnvironmentPanel
	 * @brief "ENVIRONMENT" panel: the world the viewport shows, as X-Plane 12 would have it. Date and time, place, the
	 * sun and moon that follow, visibility, clouds, rain, temperature, wind and waves, the runway surface state, the
	 * season, and presets matching the parity scenarios. Edits go straight to the world environment (Environment::Get()).
	 */
	class EnvironmentPanel : public UI::EditorPanel
	{
	public:
		explicit EnvironmentPanel(EditorContext *context);

		void OnUIRender(bool &isOpen) override;

		/* @brief Weather presets (the parity scenarios' weather): Clear, Hazy, Foggy, Overcast, Rain, Snow. */
		static void ApplyPreset(Environment::EnvironmentState &state, const char *preset);

	private:
		static bool DrawDateTime(Environment::EnvironmentState &state);
		static void DrawSunMoon(const Environment::EnvironmentDerived &derived);
		static bool DrawWeather(Environment::EnvironmentState &state);
		static bool DrawSurface(Environment::EnvironmentState &state, const Environment::EnvironmentDerived &derived);
		static bool DrawSeason(Environment::EnvironmentState &state, const Environment::EnvironmentDerived &derived);
		static void DrawSky(const Environment::EnvironmentDerived &derived);
		static bool DrawPresets(Environment::EnvironmentState &state);

		EditorContext *m_Context = nullptr;
	};
}

// -------------------------------------------------------
