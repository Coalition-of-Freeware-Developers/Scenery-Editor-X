/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* type_descriptors.h
* -------------------------------------------------------
* Created: 9/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include "type_names.h"
#include "type_utils.h"

/// -------------------------------------------------------

namespace SceneryEditorX::Types
{
	struct TDummyTag {};

	template<typename T, typename TTag = TDummyTag>
	struct Description;

	template<typename T, typename TTag = TDummyTag>
	using Described = is_specialized<Description<std::remove_cvref_t<T>, TTag>>;

	/// -------------------------------------------------------
	///		Utility wrapper to operate on a list of member pointers.
	/// -------------------------------------------------------
	template<auto... MemberPointers>
	struct MemberList
	{
        /// -------------------------------------------------------
		/// Helper type definitions
        /// -------------------------------------------------------
		using TTuple = decltype(std::tuple(MemberPointers...));

	private:
		template<size_t MemberIndex>
		using TMemberType = member_pointer::return_type<std::remove_cvref_t<decltype(std::get<MemberIndex>(TTuple()))>>::type;

		template<typename TMemberPtr>
		using TMemberPtrType = member_pointer::return_type<std::remove_cvref_t<TMemberPtr>>::type;

	public:
		using TVariant = std::variant<filter_void_t<TMemberPtrType<decltype(MemberPointers)>>...>;

		static constexpr size_t Count() { return sizeof ... (MemberPointers); }

        /// -------------------------------------------------------
		/// Call function on member list, or for each member
        /// -------------------------------------------------------

		/** Apply a function to variadic pack of the member list. */
		template<typename TObj, typename TFunc>
		static constexpr auto Apply(TFunc func, TObj& obj)
		{
			return func(obj.*MemberPointers...);
		}

		/** Apply a function to variadic pack of the member list. */
		template<typename TObj, typename TFunc>
		static constexpr auto Apply(TFunc func, const TObj& obj)
		{
			return func(obj.*MemberPointers...);
		}

		/** Apply a function to each member that's not a member function. */
		template<typename TObj, typename TFunc>
		static constexpr auto ApplyForEach(TFunc func, TObj& obj)
		{
			return (ApplyIfMemberNotFunction(func, MemberPointers, obj), ...);
		}

		/**
		 * Apply function to default initialized variables for each member type.
		 * This version does not require instance of the object of the described type.
		 * @param f		- function to apply for each member type
		 * @return		function return type
		 */
		template<typename TFunc>
		static constexpr auto ApplyToStaticType(TFunc f)
		{
			return f(MemberPointers...);
		}

	private:
		template<typename TFunc, typename TMemberPtr, typename TObj>
		static constexpr auto ApplyIfMemberNotFunction(TFunc func, TMemberPtr member, TObj& obj)
		{
			if constexpr (!std::is_member_function_pointer_v<decltype(member)>)
				func(obj.*member);
		}

		template<typename TObj, typename TFunc>
		static constexpr auto ApplyToMember(size_t memberIndex, TFunc&& f, TObj&& obj)
		{
			int memberCounter = 0;

			/// Utility to construct the 'unwrap' function and increment
			/// the counter within the variadic unpack.
			auto unwrapWithCounter = [&memberCounter, &f, memberIndex]()
			{
				auto unwrap = [memberCounter, &f, memberIndex](auto& memb)
				{
					if (std::cmp_equal(memberCounter, memberIndex))
						f(memb);
				};

				memberCounter++;

				return unwrap;
			};

			(ApplyIfMemberNotFunction(unwrapWithCounter(), MemberPointers, std::forward<TObj>(obj)), ...);
		}

		template<size_t MemberIndex, typename TObj, typename TFunc>
		constexpr static auto ApplyToMember(TFunc&& f, TObj&& obj)
		{
			f(obj.*Types::nth_element<MemberIndex>(MemberPointers...));
		}

	public:
        /// -------------------------------------------------------
		/// Set member values
        /// -------------------------------------------------------
        
		template<typename TValue, typename TObj>
		static constexpr bool SetMemberValue(size_t memberIndex, const TValue& value, TObj&& Obj)
		{
			bool valueSet = false;
			ApplyToMember(memberIndex, [&](auto& memb)
			{
				//? DBG
				//const char* idM = typeid(decltype(memb)).name();
				//const char* idV = typeid(decltype(value)).name();

				using TMemberNoCVR = std::remove_cvref_t<T0>;
				using TValueNoCVR = std::remove_cvref_t<decltype(value)>;

				if constexpr (std::is_same_v<TValueNoCVR, TMemberNoCVR>) /// SFINAE
				{
					memb = std::forward<decltype(value)>(value);
					valueSet = true;
				}
			}, std::forward<decltype(Obj)>(Obj));

			return valueSet;
		}

		template<size_t MemberIndex, typename TValue, typename TObj>
		static constexpr bool SetMemberValue(const TValue& value, TObj&& Obj)
		{
			bool valueSet = false;
			ApplyToMember<MemberIndex>([&]<typename T0>(T0& memb)
			{
				///? DBG
				///const char* idM = typeid(decltype(memb)).name();
				///const char* idV = typeid(decltype(value)).name();

				using TMemberNoCVR = std::remove_cvref_t<T0>;
				using TValueNoCVR = std::remove_cvref_t<decltype(value)>;

				if constexpr (std::is_same_v<TValueNoCVR, TMemberNoCVR>) /// SFINAE
				{
					memb = std::forward<decltype(value)>(value);
					valueSet = true;
				}
			}, std::forward<decltype(Obj)>(Obj));

			return valueSet;
		}

		/// -------------------------------------------------------
		/// Set member values
		/// -------------------------------------------------------
		template<size_t MemberIndex, typename TObj>
		static constexpr auto GetMemberValue(const TObj& obj)
		{
			static_assert(Count() > MemberIndex);

			auto filter = [&obj]<typename T0>(T0 member)
			{
				if constexpr (std::is_member_function_pointer_v<T0>)
					return;
				else
					return obj.*member;
			};
			return filter(Type::nth_element<MemberIndex>(MemberPointers...));
		}

		template<typename TValue, typename TObj>
		static constexpr std::optional<TValue> GetMemberValueOfType(size_t memberIndex, const TObj& Obj)
		{
			std::optional<TValue> value{};

			if (Count() > memberIndex)
			{
				ApplyToMember(memberIndex, [&]<typename T0>(const T0& memb)
				{
					using TMember = std::remove_cvref_t<T0>;
					//const char* typeID = typeid(TValue).name();
					//const char* typeID2 = typeid(TMember).name();

					if constexpr (std::is_same_v<TValue, TMember>) // SFINAE
                        value = memb;

                }, std::forward<decltype(Obj)>(Obj));
			}

			return value;
		}

		template<size_t MemberIndex, typename TVariantType, typename TObj>
		static constexpr TVariantType GetMemberValue(const TObj& obj)
		{
			return TVariantType(GetMemberValue<MemberIndex>(obj));
		}

		template<typename TVariantType, typename TObj>
		static constexpr TVariantType GetMemberValue(size_t memberIndex, const TObj& obj)
		{
			TVariantType variant{};
			if (Count() > memberIndex)
			{
				ApplyToMember(memberIndex,
							  [&](const auto& memb)
				{
					//using TMember = std::remove_cvref_t<decltype(memb)>;
					//const char* typeID = typeid(TValue).name();
					//const char* typeID2 = typeid(TMember).name();

					variant = TVariantType(memb);
				}, std::forward<decltype(obj)>(obj));
			}

			return variant;
		}

        /// -------------------------------------------------------
		/// Query type information
        /// -------------------------------------------------------

	    template<size_t MemberIndex>
		static constexpr auto IsFunction()
		{
			static_assert(Count() > MemberIndex);
			return std::is_member_function_pointer_v<decltype(Types::nth_element<MemberIndex>(MemberPointers...))>;
		}

		static constexpr std::optional<bool> IsFunction(size_t memberIndex)
		{
			std::optional<bool> isFunction;

			if (Count() > memberIndex)
			{
				int memberCounter = 0;
				auto unwrap = [&isFunction, memberIndex]<typename T0>(T0 memb, const int counter)
				{
					if (std::cmp_equal(counter, memberIndex))
                        isFunction = std::is_member_function_pointer_v<T0>;
                };

				(unwrap(MemberPointers, memberCounter++), ...);
			}

			return isFunction;
		}

		template<size_t MemberIndex>
		static constexpr auto GetMemberSize()
		{
			return sizeof(TMemberType<MemberIndex>);
		}

		static constexpr auto GetMemberSize(size_t memberIndex)
		{
			std::optional<size_t> size;

			if (Count() > memberIndex)
			{
				int memberCounter = 0;
				auto unwrap = [&size, memberIndex]<typename T0>(T0 memb, const int counter)
				{
					if (std::cmp_equal(counter, memberIndex))
                        size = sizeof(filter_void_t<TMemberPtrType<T0>>);
                };

				(unwrap(MemberPointers, memberCounter++), ...);
			}

			return size;
		}

		template<size_t MemberIndex>
		static constexpr auto GetTypeName()
		{
			static_assert(Count() > MemberIndex);
			return type::type_name<TMemberType<MemberIndex>>();
		}

		static constexpr auto GetTypeName(size_t memberIndex)
		{
			std::optional<std::string_view> name;

			if (Count() > memberIndex)
			{
				int memberCounter = 0;
				auto unwrap = [&name, memberIndex]<typename T0>(T0 memb, const int counter)
				{
					if (std::cmp_equal(counter, memberIndex))
                        name = type::type_name<TMemberPtrType<T0>>();
                };

				(unwrap(MemberPointers, memberCounter++), ...);
			}

			return name;
		}
	};

	template<class TDescription, class TObjType, class TTag, class TList>
	struct DescriptionInterface;

#ifndef DESCRIBED
    /// -------------------------------------------------------
	/// Description provides reflection interface with access by strings.
    /// -------------------------------------------------------

#define DESCRIBED_TAGGED(Class, Tag, ...) template<>																											\
struct SceneryEditorX::Types::Description<Class, Tag> : SceneryEditorX::Types::MemberList<__VA_ARGS__>,															\
        SceneryEditorX::Types::DescriptionInterface<SceneryEditorX::Types::Description<Class,Tag>, Class, Tag, SceneryEditorX::Types::MemberList<__VA_ARGS__>>	\
{																																								\
private:																																						\
	static constexpr std::string_view MemberStr{ #__VA_ARGS__ };																								\
	static constexpr std::string_view ClassStr{ #Class };																										\
	static constexpr std::string_view Delimiter{ ", " };																										\
																																								\
public:																																							\
	static constexpr std::string_view Namespace{ ClassStr.data(), ClassStr.find("::") == std::string_view::npos ? 0 : ClassStr.find_last_of(':') - 1 };			\
																																								\
	static constexpr std::string_view ClassName{ ClassStr.data() + (Namespace.size() ? Namespace.size() + 2 : 0) };												\
																																								\
	static constexpr std::array<std::string_view, NumberOfMembers> MemberNames = SceneryEditorX::Utils::RemoveNamespace<NumberOfMembers>(SceneryEditorX::Utils::SplitString<NumberOfMembers>(MemberStr, Delimiter));																			\
};
#endif


#ifndef DESCRIBED
#define DESCRIBED(Class, ...) DESCRIBED_TAGGED(Class, SceneryEditorX::Types::TDummyTag, __VA_ARGS__)
#endif

    /// -------------------------------------------------------
	/// A convenience interface to move some stuff out of the macro.
    /// -------------------------------------------------------
	template<class TDescription, class TObjType, class TTag, class TList>
	struct DescriptionInterface
	{
    private:
        DescriptionInterface() = default;

    public:
        static_assert(std::is_same_v<TDescription, Description<TObjType, TTag>>);
		static constexpr size_t NumberOfMembers = TList::Count();
		static constexpr size_t IndexOf(std::string_view memberName)
		{
			for (size_t i = 0; i < TDescription::Count(); ++i)
			{
				if (TDescription::MemberNames[i] == memberName)
					return i;
			}

			return TDescription::INVALID_INDEX;
		}

		static constexpr bool HasMember(std::string_view memberName)
		{
			for (const auto& name : TDescription::MemberNames)
			{
				if (name == memberName)
					return true;
			}
			return false;
		}

		static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);

		static constexpr std::optional<std::string_view> GetMemberName(size_t memberIndex)
		{
			return NumberOfMembers > memberIndex ? std::optional<std::string_view>(TDescription::MemberNames[memberIndex]) : std::nullopt;
		}

		template<size_t MemberIndex>
		static constexpr std::string_view GetMemberName()
		{
			static_assert(NumberOfMembers > MemberIndex);
			return TDescription::MemberNames[MemberIndex];
		}

		template<typename TValue>
		static constexpr std::optional<TValue> GetMemberValueByName(const std::string_view memberName, const TObjType& object)
		{
			const auto index = IndexOf(memberName);
			return NumberOfMembers > index ? TList::template GetMemberValueOfType<TValue>(index, object) : std::nullopt;
		}

		template<typename TVariantType>
		static constexpr TVariantType GetMemberValueVariantByName(const std::string_view memberName, const TObjType& object)
		{
			const auto index = IndexOf(memberName);
			return NumberOfMembers > index ? TList::template GetMemberValue<TVariantType>(index, object) : TVariantType{};
		}

		template<typename TValue>
		static constexpr bool SetMemberValueByName(const std::string_view memberName, const TValue& value, TObjType& object)
		{
			const auto index = IndexOf(memberName);
			return NumberOfMembers > index ? TList::template SetMemberValue<TValue>(index, value, object) : false;
		}

		static constexpr std::optional<bool> IsFunctionByName(const std::string_view memberName)
		{
			const auto index = IndexOf(memberName);
			return NumberOfMembers > index ? TList::IsFunction(index) : std::nullopt;
		}

		static constexpr std::optional<size_t> GetMemberSizeByName(const std::string_view memberName)
		{
			const auto index = IndexOf(memberName);
			return NumberOfMembers > index ? TList::GetMemberSize(index) : std::nullopt;
		}

		static constexpr std::optional<std::string_view> GetTypeNameByName(const std::string_view memberName)
		{
			const auto index = IndexOf(memberName);
			return NumberOfMembers > index ? std::optional<std::string_view>(TList::GetTypeName(index)) : std::nullopt;
		}

		static void Print(std::ostream& stream)
		{
			stream << "Class Name: '" << TDescription::ClassName << '\'' << '\n';
			stream << "Namespace: '" << TDescription::Namespace << '\'' << '\n';
			stream << "Number of members: " << NumberOfMembers << '\n';
			stream << "Members:" << '\n';
			stream << "---" << '\n';

			for (int i = 0; i < (int)NumberOfMembers; ++i)
			{
				stream << *TList::GetTypeName(i) << ' ' << TDescription::MemberNames[i] << ' ' << "(" << *TList::GetMemberSize(i) << " bytes)";

				if (*TList::IsFunction(i))
                    stream << " (function)";

                stream << '\n';
			}
			stream << "---" << '\n';
		}

		static void Print(std::ostream& stream, const TObjType& obj)
		{
			stream << "Class Name: '" << TDescription::ClassName << '\'' << '\n';
			stream << "Namespace: '" << TDescription::Namespace << '\'' << '\n';
			stream << "Number of members: " << NumberOfMembers << '\n';
			stream << "Members:" << '\n';
			stream << "---" << '\n';

			auto unwrapOuter = [&stream, &obj](auto... members)
			{
				int memberCounter = 0;
				auto unwrap = [&stream, &obj]<typename T0>(T0 memb, int index)
				{
					stream << *TList::GetTypeName(index) << ' ';
					stream << TDescription::MemberNames[index];

					if constexpr (!std::is_member_function_pointer_v<T0> && is_streamable_v<typename member_pointer::return_type<T0>::type>)
                        stream << "{ " << (obj.*memb) << " }";

                    stream << ' ';
					stream << "(" << *TList::GetMemberSize(index) << " bytes)";

					if (*TList::IsFunction(index))
                        stream << " (function)";

                    stream << '\n';
				};
				(unwrap(members, memberCounter++), ...);

			};
			TList::ApplyToStaticType(unwrapOuter);

			stream << "---" << '\n';
		}

        friend TDescription;
    };

	template<typename T, typename... Ts>
	std::ostream& operator<<(std::ostream& os, const std::variant<T, Ts...>& v)
	{
		std::visit([&os](auto&& arg)
		{
			os << arg;
		}, v);
		return os;
	}

    /// -------------------------------------------------------

}

/// -------------------------------------------------------
