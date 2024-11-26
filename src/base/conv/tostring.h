#ifndef CONV_TOSTRING
#define CONV_TOSTRING

#include <optional>
#include <string>
#include <type_traits>

#include "base/refl/auto_enum.h"
#include "base/type/traits.h"

#include "numtostr.h"

namespace Conv
{

template <class T>
concept ToStringMember =
    !std::is_void_v<decltype(std::declval<const T &>().toString())>;

template <class From>
constexpr inline static bool IsStringifiable =
    ToStringMember<From> || Refl::is_enum<From>
    || (Type::is_optional_v<From>
        && IsStringifiable<Type::optional_t<From>>)
    || std::is_constructible_v<std::string, From>
    || std::is_constructible_v<std::string_view, From>
    || std::is_arithmetic_v<From>;

template <> constexpr inline bool IsStringifiable<void> = false;

template <typename From>
    requires IsStringifiable<From>
[[nodiscard]] decltype(auto) toString(const From &value)
{
	if constexpr (Refl::is_enum<From>)
		return Refl::enum_name(value);
	else if constexpr (Type::is_optional_v<From>) {
		using T = std::remove_cvref_t<decltype(toString(*value))>;
		if (!value) return T{"null"};
		return T{toString(*value)};
	}
	else if constexpr (std::is_same_v<From, std::string>
	                   || std::is_same_v<From, std::string_view>)
		return (value);
	else if constexpr (std::is_constructible_v<std::string, From>)
		return static_cast<std::string>(value);
	else if constexpr (std::is_constructible_v<std::string_view,
	                       From>)
		return static_cast<std::string_view>(value);
	else if constexpr (std::is_same_v<From, bool>)
		return value ? "true" : "false";
	else if constexpr (std::is_arithmetic_v<From>)
		return std::to_string(value);
	else {
		static_assert(ToStringMember<From>);
		return value.toString();
	}
}

}

#endif
