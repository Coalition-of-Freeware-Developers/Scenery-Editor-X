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
 * iresource.cpp
 * -------------------------------------------------------
 * Created: 16/02/2026
 * -------------------------------------------------------
 */
#include "iresource.h"
#include "SceneryEditorX/renderer/vulkan/model.h"

// -----------------------------------------------------------------

namespace SceneryEditorX
{
	
	IResource::IResource(const ResourceType type)
	{
	    m_ResourceType = type;
	}
	
	template <typename T>
	ResourceType IResource::TypeToEnum()
	{
	    return ResourceType::Unknown;
	}
	
	template <typename T>
    static constexpr void ValidateResourceType()
	{
	    static_assert(std::is_base_of_v<IResource, T>, "Provided type does not implement IResource");
	}
	
	// Explicit template instantiation
	#define INSTANTIATE_TO_RESOURCE_TYPE(T, enumT)                                                                         \
	    template <>                                                                                                        \
	    ResourceType IResource::TypeToEnum<T>()                                                                            \
	    {                                                                                                                  \
	        ValidateResourceType<T>();                                                                                     \
	        return enumT;                                                                                                  \
	    }
	
	// To add a new resource to the engine, simply register it here
	//INSTANTIATE_TO_RESOURCE_TYPE(Image, ResourceType::Texture)
	//INSTANTIATE_TO_RESOURCE_TYPE(Material, ResourceType::Material)
	//INSTANTIATE_TO_RESOURCE_TYPE(Animation, ResourceType::Animation)
	//INSTANTIATE_TO_RESOURCE_TYPE(Font, ResourceType::Font)
	//INSTANTIATE_TO_RESOURCE_TYPE(Model, ResourceType::Model)
	
}

// -----------------------------------------------------------------
