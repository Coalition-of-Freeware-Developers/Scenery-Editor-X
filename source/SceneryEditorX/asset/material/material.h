/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* material.h
* -------------------------------------------------------
* Created: 11/8/2025
* -------------------------------------------------------
*/
#pragma once
#include <SceneryEditorX/renderer/shaders/shader.h>
#include <SceneryEditorX/renderer/texture.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	enum class MaterialFlag
	{
	    None				 = BIT(0),
	    DepthTest			 = BIT(1),
	    Blend				 = BIT(2),
	    TwoSided			 = BIT(3),
	    DisableShadowCasting = BIT(4)
	};

    class Material : public RefCounted
    {
    public:
        Material(const Ref<Shader> &shader, const std::string &name = "");
        Material(Ref<Material> &material, const std::string &name = "");
        virtual ~Material();

        Ref<Material> Create(const Ref<Shader> &shader, const std::string &name = "");
        Ref<Material> Copy(const Ref<Material> &other, const std::string &name = "");

        void Prepare();
		void Invalidate();
		void OnShaderReloaded();

		// -------------------------------------------------------

		void Set(const std::string &name, float value);
        void Set(const std::string &name, int value);
        void Set(const std::string &name, uint32_t value);
        void Set(const std::string &name, bool value);
        void Set(const std::string &name, const iVec2 &value);
        void Set(const std::string &name, const iVec3 &value);
        void Set(const std::string &name, const iVec4 &value);
        void Set(const std::string &name, const Vec2 &value);
        void Set(const std::string &name, const Vec3 &value);
        void Set(const std::string &name, const Vec4 &value);
        void Set(const std::string &name, const Mat3 &value);
        void Set(const std::string &name, const Mat4 &value);

        void Set(const std::string &name, const Ref<Texture2D> &texture, uint32_t arrayIndex = 0);
        void Set(const std::string &name, const Ref<TextureCube> &texture, uint32_t arrayIndex = 0);
        void Set(const std::string &name, const Ref<Image2D> &image, uint32_t arrayIndex = 0);
        void Set(const std::string &name, const Ref<ImageView> &image, uint32_t arrayIndex = 0);

        // -------------------------------------------------------

        Vec2 &GetVector2(const std::string &name);
        Vec3 &GetVector3(const std::string &name);
        Vec4 &GetVector4(const std::string &name);
        Mat3 &GetMatrix3(const std::string &name);
        Mat4 &GetMatrix4(const std::string &name);

        bool &GetBool(const std::string &name);
        float &GetFloat(const std::string &name);
        int32_t &GetInt(const std::string &name);
        uint32_t &GetUInt(const std::string &name);

        // -------------------------------------------------------

		template <typename T>
        void Set(const std::string &name, const T &value)
        {
            auto decl = FindUniformDeclaration(name);
            SEDX_CORE_ASSERT(decl, "Could not find uniform!");
            if (!decl)
                return;

            auto &buffer = m_UniformStorageBuffer;
            buffer.Write((byte *)&value, decl->GetSize(), decl->GetOffset());
        }

        template <typename T>
        T &Get(const std::string &name)
        {
            auto decl = FindUniformDeclaration(name);
            SEDX_CORE_ASSERT(decl, "Could not find uniform with name 'x'");
            auto &buffer = m_UniformStorageBuffer;
            return buffer.Read<T>(decl->GetOffset());
        }

        template <typename T>
        Ref<T> GetResource(const std::string &name)
        {
            return m_DescriptorSetManager.GetInput<T>(name);
        }

        template <typename T>
        Ref<T> TryGetResource(const std::string &name)
        {
            return m_DescriptorSetManager.GetInput<T>(name);
        }

        // -------------------------------------------------------

        uint32_t GetFlags() const { return m_MaterialFlags; }
        void SetFlags(uint32_t flags) { m_MaterialFlags = flags; }
        bool GetFlag(MaterialFlag flag) const { return (uint32_t)flag & m_MaterialFlags; }
        void SetFlag(MaterialFlag flag, bool value = true)
        {
            if (value)
            {
                m_MaterialFlags |= (uint32_t)flag;
            }
            else
            {
                m_MaterialFlags &= ~(uint32_t)flag;
            }
        }

		// -------------------------------------------------------

		Ref<Shader> GetShader() const { return m_Shader; }
		const std::string &GetName() const { return m_Name; }
        const ShaderUniform *FindUniformDeclaration(const std::string &name);
        const ShaderResourceDeclaration *FindResourceDeclaration(const std::string &name);
        Buffer GetUniformStorageBuffer() { return m_UniformStorageBuffer; }

        // -------------------------------------------------------

        VkDescriptorSet GetDescriptorSet(uint32_t index)
        {
            if (m_DescriptorSetManager.GetFirstSetIndex() == UINT32_MAX)
                return nullptr;

            Prepare();
            return m_DescriptorSetManager.GetDescriptorSets(index)[0];
        }

        // -------------------------------------------------------

    private:
        void Init();
        void AllocateStorage();

        // -------------------------------------------------------

        void SetDescriptor(const std::string &name, const Ref<Texture2D> &texture, uint32_t arrayIndex = 0);
        void SetDescriptor(const std::string &name, const Ref<TextureCube> &texture, uint32_t arrayIndex = 0);
        void SetDescriptor(const std::string &name, const Ref<Image2D> &image, uint32_t arrayIndex = 0);
        void SetDescriptor(const std::string &name, const Ref<ImageView> &image, uint32_t arrayIndex = 0);

        // -------------------------------------------------------

        Ref<Shader> m_Shader;
        std::string m_Name;
        uint32_t m_MaterialFlags = 0;
        Buffer m_UniformStorageBuffer;
    };

    

}

// -------------------------------------------------------
