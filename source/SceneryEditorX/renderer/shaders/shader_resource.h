/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader_resource.h
* -------------------------------------------------------
* Created: 8/6/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{
    class ShaderResource
    {
    public:
		ShaderResource() = default;
        ShaderResource(std::string name, uint32_t set, uint32_t resourceRegister, uint32_t count)
            : name(std::move(name)), set(set), Register(resourceRegister), count(count) { }

		[[nodiscard]] virtual const std::string& GetName() const { return name; }
        [[nodiscard]] virtual uint32_t GetSet() const { return set; }
		[[nodiscard]] virtual uint32_t GetRegister() const { return Register; }
		[[nodiscard]] virtual uint32_t GetCount() const { return count; }

    private:
        std::string name;
        uint32_t set = 0;
        uint32_t Register = 0;
        uint32_t count = 0;
    };

    typedef std::vector<ShaderResource *> ShaderResourceList;

}

/// -------------------------------------------------------
