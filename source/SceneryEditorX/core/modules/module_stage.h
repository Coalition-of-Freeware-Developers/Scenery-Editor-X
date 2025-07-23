/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* module_stage.h
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/core/modules/module.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	class ModuleStage
	{
	public:

        ModuleStage() = default;
        ~ModuleStage() = default;

		void PushModule(Module *module);
        void PushOverlay(Module *overlay);
        void PopModule(Module *module);
        void PopOverlay(Module *overlay);

		Module *operator[](size_t index);
        const Module *operator[](size_t index) const;

        [[nodiscard]] size_t Size() const { return moduleStage.size(); }

		std::vector<Module*>::iterator begin() { return moduleStage.begin(); }
		std::vector<Module*>::iterator end() { return moduleStage.end(); }

	private:
        std::vector<Module*> moduleStage;
        unsigned int moduleInsertIndex = 0;
	};
}

/// -------------------------------------------------------
