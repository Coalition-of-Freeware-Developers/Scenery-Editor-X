/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* animation.h
* -------------------------------------------------------
* Created: 15/4/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/scene/asset.h>

// -------------------------------------------------------
namespace SceneryEditorX
{

	class Animation : public Asset
	{
	public:
	    Animation();
	    virtual ~Animation() override;
	
	    // -------------------------------------------------------
	
	    //virtual void Load(const std::string &path) override;
	    //virtual void Unload() override;
	    //virtual bool IsLoaded() const;
	    virtual const std::string &GetPath() const;
	    virtual const std::string &GetName() const;
	    //virtual void SetName(const std::string &name) override;

    private:
        friend class AssetManager;
	};
	

} // namespace SceneryEditorX

// -------------------------------------------------------
