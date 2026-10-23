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
 * shader_uniform.h
 * -------------------------------------------------------
 * Created: 03/07/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <functional>
#include <ranges>
#include <SceneryEditorX/logging/asserts.h>
#include <SceneryEditorX/renderer/vulkan/descriptor.h>
#include <SceneryEditorX/utils/filestreaming/filestream_reader.h>
#include <SceneryEditorX/utils/filestreaming/filestream_writer.h>
#include <SceneryEditorX/utils/pointers.h>
#include <unordered_map>

// -------------------------------------------------------

namespace SceneryEditorX 
{

	enum class ShaderDomain
	{
		None	 = 0, 
		Vertex	 = 0,
		Fragment = 1 // unused
	};

	/**
	 * @class ShaderResourceDeclaration
	 * @brief Represents a reflected shader resource binding: name, descriptor set, binding (register),
	 * array count, descriptor type and the Vulkan shader stages that actually access it.
	 */
	class ShaderResourceDeclaration
	{
	public:
		ShaderResourceDeclaration() = default;
		ShaderResourceDeclaration(const std::string& name, uint32_t set, uint32_t resourceRegister, uint32_t count,
								  DescriptorType type = DescriptorType::MaxEnum, uint32_t stageFlags = 0) :
			m_Name(name), m_Set(set), m_Register(resourceRegister), m_Count(count), m_Type(type), m_StageFlags(stageFlags) { }

		virtual ~ShaderResourceDeclaration() = default;

		virtual const std::string& GetName() const { return m_Name; }
		virtual uint32_t GetSet() const { return m_Set; }
		virtual uint32_t GetRegister() const { return m_Register; }
		virtual uint32_t GetCount() const { return m_Count; }
		[[nodiscard]] DescriptorType GetType() const { return m_Type; }

		/** @brief VkShaderStageFlags of the stages that reference this resource. */
		[[nodiscard]] uint32_t GetStageFlags() const { return m_StageFlags; }
		void AddStageFlags(uint32_t stageFlags) { m_StageFlags |= stageFlags; }

		static void Serialize(StreamWriter* serializer, const ShaderResourceDeclaration& instance)
		{
			serializer->WriteString(instance.m_Name);
			serializer->WriteRaw(instance.m_Set);
			serializer->WriteRaw(instance.m_Register);
			serializer->WriteRaw(instance.m_Count);
			serializer->WriteRaw(instance.m_Type);
			serializer->WriteRaw(instance.m_StageFlags);
		}

		static void Deserialize(StreamReader* deserializer, ShaderResourceDeclaration& instance)
		{
			deserializer->ReadString(instance.m_Name);
			deserializer->ReadRaw(instance.m_Set);
			deserializer->ReadRaw(instance.m_Register);
			deserializer->ReadRaw(instance.m_Count);
			deserializer->ReadRaw(instance.m_Type);
			deserializer->ReadRaw(instance.m_StageFlags);
		}

	private:
		std::string m_Name;
		uint32_t m_Set = 0;
		uint32_t m_Register = 0;
		uint32_t m_Count = 0;
		DescriptorType m_Type = DescriptorType::MaxEnum;
		uint32_t m_StageFlags = 0;
	};

	typedef std::vector<ShaderResourceDeclaration*> ShaderResourceList;

}

// -------------------------------------------------------
