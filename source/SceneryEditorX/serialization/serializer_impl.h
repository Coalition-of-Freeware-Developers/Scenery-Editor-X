/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* serializer_impl.h
* -------------------------------------------------------
* Created: 9/7/2025
* -------------------------------------------------------
*/
#pragma once
#include "serializer_reader.h"
#include "serializer_writer.h"
#include "SceneryEditorX/core/identifiers/identifier.h"
#include "SceneryEditorX/utils/reflection/type_descriptors.h"

/// -------------------------------------------------------
	
	/**
	 *  Non-intrusive Binary / Memory Serialization Interface.
	 *
	 *	Add SERIALIZABLE macro in a header for class you need to be serialized
	 *	member by member. For example:
	 *
	 *	SERIALIZABLE(MyClass,
	 *				&MyClass::MemberPrimitive,
	 *				&MyClass::MemberVector,
	 *				&MyClass::MemberAnotherSerializable);
	 *
	 *	If there's no spatialization for the ..Impl (de)serialize function for any of the listed in the macro members, it's going to call:
	 *
	 *		T::Serialize(StreamWriter* writer, T& obj)
	 *	..or
	 *		T::Deserialize(StreamReader* reader, T& obj)
	 *
	 *	..where T is the Types of the object that need to have Serialized and Deserialize
	 *	functions with this signature.
	 *
	 *	Instead of adding intrusive interface as described above you can add to your
	 *	header specialization for the free template functions as follows:
	 *
	 *	@code
	 *	namespace SceneryEditorX::Spatialization::Impl
	 *	{
	 *		template<>
	 *		static inline bool SerializeImpl(StreamWriter* writer, const MyClass& v)
	 *		{
	 *			... some custom serialization routine ...
	 *			writer->WriteRaw(42069);
	 *			return true;
	 *		}
	 *
	 *		template<>
	 *		static inline bool DeserializeImpl(StreamReader* reader, MyClass& v)
	 *		{
	 *		... some custom deserialization routine ...
	 *		reader->ReadRaw(v.MyVariable);
	 *		return true;
	 *		}
	 *	}
	 */

/*
#define SERIALIZABLE(Class, ...)                                                                                       \
    template <>                                                                                                        \
	struct SceneryEditorX::Types::Description<Class> : SceneryEditorX::Types::MemberList<__VA_ARGS__>                  \
	{};
	*/
	
namespace SceneryEditorX::Serialization
{/*

	/// Serialization
	template <typename... Ts>
    static bool Serialize(SerializeWriter *writer, const Ts &...vs);
	
	namespace Impl
	{
		template <typename T>
		static bool SerializeImpl(SerializeWriter *writer, const T &v)
		{
		    if constexpr (std::is_trivial_v<T>)
		        writer->WriteRaw(v);
		    else
		        T::Serialize(writer, v);
		
		    return true;
		}
		
		/// ----------------------------------------------------------------------------
		/// Serialization specialization More default specializations can be added...
		/// ----------------------------------------------------------------------------
		
		template <>
		SEDX_EXPLICIT_STATIC inline bool SerializeImpl(SerializeWriter *writer, const UUID &v)
		{
		    writer->WriteRaw((AssetHandle)v);
		    return true;
		}
		
		template <>
		SEDX_EXPLICIT_STATIC inline bool SerializeImpl(SerializeWriter *writer, const Identifier &v)
		{
		    writer->WriteRaw((uint32_t)v);
		    return true;
		}
		
		template <>
		SEDX_EXPLICIT_STATIC inline bool SerializeImpl(SerializeWriter *writer, const std::string &v)
		{
		    writer->WriteString(v);
		    return true;
		}
		
		/// ----------------------------------------------------------------------------

		template <typename T>
		SEDX_EXPLICIT_STATIC bool SerializeVec(SerializeWriter *writer, const std::vector<T> &vec)
		{
		    //if (writeSize)
		    writer->WriteRaw<uint32_t>((uint32_t)vec.size());
		
		    for (const auto &element : vec)
		        Serialize(writer, element);
		
		    return true;
		}
		
		/// ----------------------------------------------------------------------------

		template <typename T>
		SEDX_EXPLICIT_STATIC bool SerializeByType(SerializeWriter *writer, const T &v)
		{
		    if constexpr (Types::Described<T>::value)
		        return Types::Description<T>::Apply([&writer](const auto &...members) { return Serialize(writer, members...); }, v);
		    else if constexpr (Types::is_array_v<T>)
		        return SerializeVec(writer, v);
		    else
		        return SerializeImpl(writer, v);
		}
	} /// namespace Impl
	
	template <typename... Ts>
    static bool Serialize(SerializeWriter *writer, const Ts &...vs)
	{
	    return (Impl::SerializeByType(writer, vs) && ...);
	}

	/// -----------------------------------------------------------------------------

    /// Deserialization
	template <typename... Ts>
    static bool Deserialize(SerializeReader *reader, Ts &...vs);
	
	namespace Impl
	{
	template <typename T>
    static bool DeserializeImpl(SerializeReader *reader, T &v)
	{
	    if constexpr (std::is_trivial_v<T>)
	        reader->ReadRaw(v);
	    else
	        T::Deserialize(reader, v);
	
	    return true;
	}
	
	/// ----------------------------------------------------------------------------
    /// Deserialization specialization More default specializations can be added...
	/// ----------------------------------------------------------------------------
	
	template <>
    SEDX_EXPLICIT_STATIC inline bool DeserializeImpl(SerializeReader *reader, UUID &v)
	{
	    reader->ReadRaw(v);
	    return true;
	}
	
	template <>
    SEDX_EXPLICIT_STATIC inline bool DeserializeImpl(SerializeReader *reader, Identifier &v)
	{
        Identifier data;
	    reader->ReadRaw(data);
	    v = data;
	    return true;
	}
	
	template <>
    SEDX_EXPLICIT_STATIC inline bool DeserializeImpl(SerializeReader *reader, std::string &v)
	{
	    reader->ReadString(v);
	    return true;
	}
	
	/// ----------------------------------------------------------------------------

	template <typename T>
    SEDX_EXPLICIT_STATIC bool DeserializeVec(SerializeReader *reader, std::vector<T> &vec)
	{
	    uint32_t size = 0;
	    //if (size == 0)
	    reader->ReadRaw<uint32_t>(size);
	
	    vec.resize(size);
	
	    for (uint32_t i = 0; i < size; i++)
	        Deserialize(reader, vec[i]);
	
	    return true;
	}
	
	/// ----------------------------------------------------------------------------

	template <typename T>
    SEDX_EXPLICIT_STATIC bool DeserializeByType(SerializeReader *reader, T &v)
	{
	    if constexpr (Types::Described<T>::value)
            return Types::Description<T>::Apply([&reader](auto &...members) { return Deserialize(reader, members...); }, v);
        else if constexpr (Types::is_array_v<T>)
            return DeserializeVec(reader, v);
        else
            return DeserializeImpl(reader, v);
    }
	} /// namespace Impl
	
	template <typename... Ts>
    static bool Deserialize(SerializeReader *reader, Ts &...vs)
	{
	    return (Impl::DeserializeByType(reader, vs) && ...);
	}

    /// -----------------------------------------------------------------------------
    */

}

/// -------------------------------------------------------
