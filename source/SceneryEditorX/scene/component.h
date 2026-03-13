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
#include "component_sets.h"
#include <any>
#include <SceneryEditorX/utils/inheritance.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Entity;

/**
	 * @struct ComponentType
	 * @brief Represents the type of a component.
	 */
	enum class ComponentType : uint32_t
	{
		Camera,
		Light,
		Renderable,
		Spline,
		Terrain,
		Volume,
		Script,
		Plugin,
		ParticleSystem,
		MaxEnum
	};

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
	class Component : SharedObject
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

		/* @brief Called every frame, before Tick, useful to reset states before the main update. */
		virtual void PreTick() {}

		/* @brief Called every frame to update the component's state. */
		virtual void Tick() {}

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
		template<typename T>
		static ComponentType TypeToEnum();

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
		[[nodiscard]] Entity * GetEntity() const { return m_EntityPtr; }

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

		ComponentType m_Type = ComponentType::MaxEnum; // The type of the component
		bool m_Enabled = false; // The state of the component
		Entity* m_EntityPtr = nullptr; // The owner of the component

	private:
		std::vector<Attribute> m_Attributes; // The attributes of the component
	};

}

// -------------------------------------------------------
