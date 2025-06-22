/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* module_stage.cpp
* -------------------------------------------------------
* Created: 22/6/2025
* -------------------------------------------------------
*/
#include <SceneryEditorX/core/modules/module_stage.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	void ModuleStage::PushModule(Module *module)
	{
        moduleStage.emplace(moduleStage.begin() + moduleInsertIndex, module);
        moduleInsertIndex++;
	}
	
	void ModuleStage::PushOverlay(Module *overlay)
	{
        moduleStage.emplace_back(overlay);
	}
	
	void ModuleStage::PopModule(Module *module)
	{
        if (const auto set = std::ranges::find(moduleStage, module); set != moduleStage.end())
		{
			moduleStage.erase(set);
			moduleInsertIndex--;
		}
		else
            SEDX_CORE_ERROR("Module not found in stage");
    }
	
	void ModuleStage::PopOverlay(Module *overlay)
	{
        if (const auto set = std::ranges::find(moduleStage.begin(), moduleStage.end(), overlay); set != moduleStage.end())
            moduleStage.erase(set);
        else
            SEDX_CORE_ERROR("Overlay not found in stage");
	}
}

/// -------------------------------------------------------
