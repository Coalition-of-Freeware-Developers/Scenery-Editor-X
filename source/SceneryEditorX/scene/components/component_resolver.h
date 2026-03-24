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
 * component_resolver.h
 * -------------------------------------------------------
 * Created: 15/03/2026
 * -------------------------------------------------------
 */
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{

	class Camera;
	class Light;
	class ParticleSystem;
	class Plugin;
	class Renderable;
	class Script;
	class Spline;
	class Submesh;
	class Terrain;
	class Volume;

	/**
	 * @struct ComponentType
	 * @brief Represents the type of component.
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
		Submesh,
		ParticleSystem,
		MaxEnum
	};

	/**
	 * @brief Compile-time resolver that maps C++ component types to ComponentType values.
	 * @tparam T Component C++ type.
	 */
	template <typename T>
	struct ComponentTypeResolver
	{
		static constexpr bool IS_REGISTERED = false;
		static constexpr ComponentType TYPE = ComponentType::MaxEnum;
	};

	/**
	 * @brief 
	 * @tparam T 
	 */
	template <>
	struct ComponentTypeResolver<Camera>
	{
		static constexpr bool IS_REGISTERED = true;
		static constexpr ComponentType TYPE = ComponentType::Camera;
	};

	/**
	 * @brief 
	 * @tparam T 
	 */
	template <>
	struct ComponentTypeResolver<Light>
	{
		static constexpr bool IS_REGISTERED = true;
		static constexpr ComponentType TYPE = ComponentType::Light;
	};

	/**
	 * @brief 
	 * @tparam T 
	 */
	template <>
	struct ComponentTypeResolver<Submesh>
	{
		static constexpr bool IS_REGISTERED = false;
		static constexpr ComponentType TYPE = ComponentType::Submesh;
	};

	/**
	 * @brief 
	 * @tparam T 
	 */
	template <>
	struct ComponentTypeResolver<Renderable>
	{
		static constexpr bool IS_REGISTERED = true;
		static constexpr ComponentType TYPE = ComponentType::Renderable;
	};

	/**
	 * @brief 
	 * @tparam T 
	 */
	template <>
	struct ComponentTypeResolver<Spline>
	{
		static constexpr bool IS_REGISTERED = true;
		static constexpr ComponentType TYPE = ComponentType::Spline;
	};

	/**
	 * @brief 
	 * @tparam T 
	 */
	template <>
	struct ComponentTypeResolver<Terrain>
	{
		static constexpr bool IS_REGISTERED = true;
		static constexpr ComponentType TYPE = ComponentType::Terrain;
	};

	/**
	 * @brief 
	 * @tparam T 
	 */
	template <>
	struct ComponentTypeResolver<Volume>
	{
		static constexpr bool IS_REGISTERED = true;
		static constexpr ComponentType TYPE = ComponentType::Volume;
	};

	/**
	 * @brief 
	 * @tparam T 
	 */
	template <>
	struct ComponentTypeResolver<Script>
	{
		static constexpr bool IS_REGISTERED = true;
		static constexpr ComponentType TYPE = ComponentType::Script;
	};

	/**
	 * @brief 
	 * @tparam T 
	 */
	template <>
	struct ComponentTypeResolver<Plugin>
	{
		static constexpr bool IS_REGISTERED = true;
		static constexpr ComponentType TYPE = ComponentType::Plugin;
	};

	/**
	 * @brief 
	 * @tparam T 
	 */
	template <>
	struct ComponentTypeResolver<ParticleSystem>
	{
		static constexpr bool IS_REGISTERED = true;
		static constexpr ComponentType TYPE = ComponentType::ParticleSystem;
	};

}

// -------------------------------------------------------

