/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* shader_uniforms.h
* -------------------------------------------------------
* Created: 9/7/2025
* -------------------------------------------------------
*/
#pragma once

/// -------------------------------------------------------

namespace SceneryEditorX
{
    enum class ShaderDomain : uint8_t
	{
		None = 0,
        Vertex = 0,
        Pixel = 1 // unused
	};

    /// -------------------------------------------------------

	class ShaderResourceDeclaration
	{
	public:
		ShaderResourceDeclaration() = default;
		ShaderResourceDeclaration(std::string name, uint32_t set, uint32_t resourceRegister, uint32_t count)
			: m_Name(std::move(name)), m_Set(set), m_Register(resourceRegister), m_Count(count) { }

        [[nodiscard]] virtual const std::string& GetName() const { return m_Name; }
		[[nodiscard]] virtual uint32_t GetSet() const { return m_Set; }
		[[nodiscard]] virtual uint32_t GetRegister() const { return m_Register; }
		[[nodiscard]] virtual uint32_t GetCount() const { return m_Count; }

		/*
		static void Serialize(SerializeWriter* serializer, const ShaderResourceDeclaration& instance)
		{
			serializer->WriteString(instance.m_Name);
			serializer->WriteRaw(instance.m_Set);
			serializer->WriteRaw(instance.m_Register);
			serializer->WriteRaw(instance.m_Count);
		}

		static void Deserialize(SerializeReader *deserializer, ShaderResourceDeclaration &instance)
		{
			deserializer->ReadString(instance.m_Name);
			deserializer->ReadRaw(instance.m_Set);
			deserializer->ReadRaw(instance.m_Register);
			deserializer->ReadRaw(instance.m_Count);
		}*/
	private:
		std::string m_Name;
		uint32_t m_Set = 0;
		uint32_t m_Register = 0;
		uint32_t m_Count = 0;
	};

	//typedef std::vector<ShaderResourceDeclaration*> ShaderResourceList;

}

/// -------------------------------------------------------
