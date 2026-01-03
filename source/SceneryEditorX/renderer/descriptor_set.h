/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* descriptor_set.h
* -------------------------------------------------------
* Created: 13/12/2025
* -------------------------------------------------------
*/
#pragma once
#include "descriptors.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	class DescriptorSet : public Resource
	{
    public:
        DescriptorSet() = default;
        DescriptorSet(const std::vector<Descriptors>& descriptors, DescriptorSetLayout* descriptorSetLayout, std::string* name);
        ~DescriptorSet() = default;

        bool IsRefToResource(const void * resource);
        void* GetResource() { return m_Resource; }

    private:
        void Update(const std::vector<Descriptors>& descriptors);

        std::vector<Descriptors> m_Descriptors;
        void* m_Resource = nullptr;
	};

}

// -------------------------------------------------------
