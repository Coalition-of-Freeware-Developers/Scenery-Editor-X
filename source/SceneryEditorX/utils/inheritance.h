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
 * inheritance.h
 * -------------------------------------------------------
 * Created: 26/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/asset/asset.h>
#include <SceneryEditorX/core/resource/iobject.h>
#include <SceneryEditorX/core/resource/iresource.h>
#include <SceneryEditorX/utils/pointers.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief A utility struct that allows us to create a new type that inherits from multiple base classes.
	 * @tparam Bases A variadic list of base classes to inherit from. This can be any combination of RefCounted, IObject, IResource, Asset, or any other class.
	 */
	template <typename... Bases>
	struct InheritanceBundle : Bases... 
	{
		// Bring constructors into scope if needed
		using Bases::Bases...; 
	};

	/**
	 * @brief A type that inherits from RefCounted and IObject, providing shared ownership and object interface functionality.
	 */
	typedef InheritanceBundle<RefCounted, IObject> SharedObject;

	/**
	 * @brief A type that inherits from RefCounted and IResource, providing shared ownership and resource interface functionality.
	 */
	typedef InheritanceBundle<RefCounted, IResource> SharedResource;

	/** 
	 * TODO: Consider whether we want to have a separate SharedAsset type that inherits from Asset instead of IResource.  
	 * This would allow us to have asset-specific functionality in SharedAsset, while still allowing SharedResource to be used for non-asset resources. 
	 * For now, we'll just use SharedResource for everything, but this is something to consider for future refactoring. 
	 */

	/**
	 * @brief A type that inherits from RefCounted, IResource, and Asset, providing shared ownership, resource interface functionality, and asset-specific functionality.
	 */
	typedef InheritanceBundle<IResource, Asset> SharedAsset;

}

// -------------------------------------------------------
