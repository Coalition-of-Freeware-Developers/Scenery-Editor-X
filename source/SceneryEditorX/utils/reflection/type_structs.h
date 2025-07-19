/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* type_structs.h
* -------------------------------------------------------
* Created: 9/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <string>
#include <type_traits>
#include <vector>
#include "type_descriptors.h"

/// -------------------------------------------------------

namespace SceneryEditorX::Types
{
	namespace Reflection
	{
		struct Member
		{
			std::string Name;
			size_t Size; /// 0 if function
			std::string TypeName;
	
			enum EType : uint8_t
			{
				Function,
				Data
			} Type;
	
			bool operator==(const Member& other) const
			{
				return Name == other.Name
					&& Size == other.Size
					&& TypeName == other.TypeName
					&& Type == other.Type;
			}
		};

        /// -------------------------------------------------------
	
		struct ClassInfo
		{
			std::string Name;
			size_t Size;					/// sizeof(T)
			std::vector<Member> Members;	/// "described" members
	
			template<class T>
			static ClassInfo Of()
			{
	            static_assert(Described<T>::value, "Type must be 'Described'.");
	
				/// Parse info using type Description
	
				ClassInfo info;
				using Descr = Types::Description<T>;
	
				info.Name = Descr::ClassName;
				info.Size = sizeof(T);
	
				for (const auto& memberName : Descr::MemberNames)
				{
					const bool isFunction = *Descr::IsFunctionByName(memberName);
					const auto typeName = *Descr::GetTypeNameByName(memberName);
	
					info.Members.push_back(
						Member{
							/*.Name*/{ memberName },
							/*.Size*/{ *Descr::GetMemberSizeByName(memberName) * !isFunction },
							/*.TypeName*/{ std::string_view(typeName.data(), typeName.size())},
							/*.Type*/{ isFunction ? Member::Function : Member::Data }
						});
				}
	
				return info;
			}
	
			bool operator==(const ClassInfo& other) const
			{
				return Name == other.Name
					&& Size == other.Size
					&& Members == other.Members;
			}
		};

	}

    /// -------------------------------------------------------

	struct TestStruct
	{
		int i = 5;
		float f = 3.2f;
		char ch = 'c';
		int* pi = nullptr;

        static void vfunc() {}
	    [[nodiscard]] static bool bfunc()
        { return true; }
	};

    /// -------------------------------------------------------

	namespace Type
	{
		DESCRIBED(TestStruct, &TestStruct::i, &TestStruct::f, &TestStruct::ch, &TestStruct::pi, &TestStruct::vfunc, &TestStruct::bfunc);
	}

    /// -------------------------------------------------------

	namespace Reflection
	{
        inline bool ClassInfoTest()
		{
			static const ClassInfo cl = ClassInfo::Of<TestStruct>();
	
			static const ClassInfo ExpectedInfo
			{
				/*.Name =*/
	            .Name = "TestStruct",
				/*.Size =*/
	            .Size = 24,
				/*.Members =*/
	            .Members = std::vector<Member>
				{
					{
					    /*.Name =*/ .Name = "i",
					    /*.Size =*/ .Size = 4,
					    /*.TypeName =*/ .TypeName = "int",
					    /*.Type =*/.Type = Member::Data
					},
	
					{
					    /*.Name =*/ .Name = "f",
					    /*.Size =*/ .Size = 4,
					    /*.TypeName =*/ .TypeName = "float",
					    /*.Type =*/ .Type = Member::Data
					},
	
					{
					    /*.Name =*/ .Name = "ch",
					    /*.Size =*/ .Size = 1,
					    /*.TypeName =*/ .TypeName = "char",
					    /*.Type =*/ .Type = Member::Data
					},
	
					{
					    /*.Name =*/ .Name = "pi",
					    /*.Size =*/ .Size = 8,
					    /*.TypeName =*/ .TypeName = "int*",
					    /*.Type =*/ .Type = Member::Data
					},
	
					{
					    /*.Name =*/ .Name = "vfunc",
					    /*.Size =*/ .Size = 0,
					    /*.TypeName =*/ .TypeName = "void",
					    /*.Type =*/ .Type = Member::EType::Function
					},
	
					{
					    /*.Name =*/ .Name = "bfunc",
					    /*.Size =*/ .Size = 0,
					    /*.TypeName =*/ .TypeName = "bool",
					    /*.Type =*/ .Type = Member::EType::Function
					},
				}
			};
	
			return cl == ExpectedInfo;
		}
	}
	
}

/// -------------------------------------------------------
