/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* plugin_system.h
* -------------------------------------------------------
* Created: 19/7/2025
* -------------------------------------------------------
*/
#pragma once

/// --------------------------------------------

namespace SceneryEditorX::Plugin
{
	class PluginManager;

    /// --------------------------------------------

	class Plugin
	{
	public:
		virtual ~Plugin() = default;
		/** Called when the plugin is loaded. */
		virtual void OnLoad(PluginManager &manager) = 0;
		/** Called when the plugin is unloaded. */
		virtual void OnUnload(PluginManager &manager) = 0;
	};

	/// --------------------------------------------

}

/// --------------------------------------------
