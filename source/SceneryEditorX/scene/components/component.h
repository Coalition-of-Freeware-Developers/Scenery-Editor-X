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
 * component.h
 * -------------------------------------------------------
 * Created: 11/03/2026
 * -------------------------------------------------------
 */
#pragma once
#include "component_resolver.h"
#include "component_sets.h"
#include <any>
#include <string_view>
#include <type_traits>
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Entity;

	/**
	 * @struct Attribute
	 * @brief Represents an attribute of a component.
	 */
	struct Attribute
	{
		std::function<std::any()> get;
		std::function<void(std::any)> set;
	};

	/**
	 * @class Component
	 * @brief Represents a component in the scene.
	 */
	class Component : public SharedObject
	{
	public:
		/**
		 * @brief Constructs a new Component object.
		 * @param entity A pointer to the entity that owns this component.
		 */
		Component(Entity* entity);
		virtual ~Component() = default;

		/* @brief Called when the component gets added to an entity. */
		virtual void Initialize() {}

		/* @brief Called every time the simulation starts. */
		virtual void Start() {}

		/* @brief Called every time the simulation stops. */
		virtual void Stop() {}

		/* @brief Called when the component is removed from the entity. */
		virtual void Remove() {}

		/**
		 * @brief Gets the type of the component.
		 * @return The ComponentType enum value representing the type of the component.
		 */
		[[nodiscard]] ComponentType GetType() const { return m_Type; }

		/**
		 * @brief Sets the type of the component.
		 * @param type The type to set for the component.
		 */
		void SetType(ComponentType type) { m_Type = type; }

		/**
		 * @brief Maps a Component subclass type T to its ComponentType enum value.
		 * @tparam T A type derived from Component.
		 * @return The ComponentType enum value corresponding to T.
		 * @note This function must be explicitly specialized for each concrete Component subclass.
		 */
		template <typename T>
		static ComponentType TypeToEnum()
		{
		  static_assert(ComponentTypeResolver<T>::IS_REGISTERED,
				"TypeToEnum<T>: T is not a registered SEDX ComponentType / ComponentTypeResolver");
			return ComponentTypeResolver<T>::TYPE;
		}

		/**
		 * @brief Converts a ComponentType value to its string token.
		 * @param type The component type.
		 * @return Lowercase/snake_case token for the type, or an empty string_view if invalid.
		 */
		[[nodiscard]] static constexpr std::string_view TypeToString(const ComponentType type)
		{
			switch (type)
			{
			    case ComponentType::Camera:			return "camera";
				case ComponentType::Light:			return "light";
				case ComponentType::Renderable:		return "renderable";
				case ComponentType::Spline:			return "spline";
				case ComponentType::Terrain:		return "terrain";
				case ComponentType::Volume:			return "volume";
				case ComponentType::Script:			return "script";
				case ComponentType::Plugin:			return "plugin";
				case ComponentType::ParticleSystem: return "particle_system";
				default:
					return {};
			}
		}

		/**
		 * @brief Converts a string token to a ComponentType value.
		 * @param name Lowercase/snake_case component token.
		 * @return Matching ComponentType, or ComponentType::MaxEnum if unknown.
		 */
		[[nodiscard]] static constexpr ComponentType StringToType(const std::string_view name)
		{
		    if (name == "camera")			return ComponentType::Camera;
			if (name == "light")			return ComponentType::Light;
			if (name == "renderable")		return ComponentType::Renderable;
			if (name == "spline")			return ComponentType::Spline;
			if (name == "terrain")			return ComponentType::Terrain;
			if (name == "volume")			return ComponentType::Volume;
			if (name == "script")			return ComponentType::Script;
			if (name == "plugin")			return ComponentType::Plugin;
			if (name == "particle_system")	return ComponentType::ParticleSystem;

			return ComponentType::MaxEnum;
		}

		/**
		 * @brief Gets the attributes of the component.
		 * @return A constant reference to the vector of attributes.
		 */
		[[nodiscard]] const auto& GetAttributes() const { return m_Attributes; }

		/**
		 * @brief Sets the attributes of the component. The input vector should have the same size and order as the existing attributes.
		 * @param attributes A vector of Attribute structs containing getter and setter functions for each attribute.
		 * @note The size and order must match the existing attributes of the component.
		 */
		void SetAttributes(const std::vector<Attribute>& attributes)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(m_Attributes.size()); i++)
			{
				m_Attributes[i].set(attributes[i].get());
			}
		}

		/**
		 * @brief Gets the entity that owns this component.
		 * @return A pointer to the owning entity.
		 */
		[[nodiscard]] Entity *GetEntity() const { return m_EntityPtr; }

	protected:
		/**
		 * @brief Registers an attribute for the component.
		 * @param getter A function that returns the current value of the attribute.
		 * @param setter A function that sets the value of the attribute.
		 */
		void RegisterAttribute(std::function<std::any()>&& getter, std::function<void(std::any)>&& setter)
		{
			Attribute attribute;
			attribute.get = std::move(getter);
			attribute.set = std::move(setter);
			m_Attributes.emplace_back(attribute);
		}

		bool m_Enabled		 = false;					// The state of the component
		Entity* m_EntityPtr  = nullptr;					// The owner of the component
		ComponentType m_Type = ComponentType::MaxEnum;	// The type of the component

	private:
		std::vector<Attribute> m_Attributes; // The attributes of the component
	};

} // namespace SceneryEditorX

// -------------------------------------------------------
