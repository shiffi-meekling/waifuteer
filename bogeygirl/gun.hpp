#pragma once
/** USAGE: 
```
#include "gun.hpp"
// your header which overloads `bogeygirl::gun::convert` goes here
BOGEYGIRL_GUN_EXTEND_END
```

when you wish to call bogeygirl::gun::convert while overloading
bogeygirl::gun::convert, call `bogeygirl::gun::convert_delay_define` instead.

`convert_delay_define` is defined when `BOGEYGIRL_GUN_EXTEND_END` is called, 
which prevents recursive converts from only being able to call coverts define
before themselves. `convert_delay_define` is defined to merely call `convert` 

called 'gun' because it coercers token trees into values
*/

#include <algorithm>
#include <boost/core/demangle.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/pfr/core_name.hpp>
#include <boost/pfr/tuple_size.hpp>
#include <concepts>
#include <string>
#include <string_view>
#include <print>
#include <tuple>
#include <variant>
#include <vector>
#include <optional>
#include <cstdlib>
#include <type_traits>
#include <boost/lexical_cast.hpp>
#include <boost/pfr.hpp>
#include <unordered_map>
#include <utility>
#include "bogeygirl.hpp"
#include <stdexcept>
#include "strlib.hpp"


namespace bogeygirl::gun {

	namespace typenames {
		template<class T> constexpr std::string get_name() { 
			return boost::core::demangle(typeid(T).name());
		}
	};

	/// a weaker same_as assertion
	template<typename A, typename B>
	concept Sameish = std::same_as<
			std::remove_cvref_t<A>,
			std::remove_cvref_t<B>
	>;

	/// something that has most of the features of an unordered_map in C++
	template<class T> concept MinimalMap = requires (T map) {
		typename T::key_type;
		typename T::mapped_type;
		map.begin();
		{ map.begin()->first } -> Sameish<typename T::key_type>;
		{ map.begin()->second } -> Sameish<typename T::mapped_type>;
		map.end();
		{ map.operator[](typename T::key_type()) } -> Sameish<typename T::mapped_type>;
	};
	static_assert(MinimalMap<std::pmr::unordered_map<int,float>>);

	///type to identify Types which are Hint Type
	///I.e. types which apply a transform, but do not appear in the result
	///set the typename ::kind to TypeHintToken, to make your type a type hint
	///sadly there can only be one template paramater
	struct TypeHintToken {};

	struct HintTypeInterfaceTesterTestType {};
	///No-op only useful implementing concepts
	template<template<class> class Hint, class T> constexpr void 
		HintTypeInterfaceTester(Hint<T> foo)  {
			//Note that this conversion is not expected to preseve the value of foo.value
			// a.value is default constructed here
			Hint<HintTypeInterfaceTesterTestType> a = foo;
		}

	template<class T> concept IsHintType = 
		std::same_as<typename T::kind, TypeHintToken>
		&& requires (T a) {
			a.value;
			HintTypeInterfaceTester(a);
		};

	template<class T> concept NotHintType = !IsHintType<T>;

	auto constexpr stripHintTypes(auto a) {
		return a;
	}

	auto constexpr stripHintTypes(IsHintType auto a) {
		return stripHintTypes(a.value);
	}

	template<class T> using stripHintTypes_t 
		= decltype(stripHintTypes(std::declval<T>()));

	//These would work but put the Hint Types unside down
	//base case
	template<template<class> class Hint, NotHintType T, class TargetT>
	requires IsHintType<Hint<T>>
	constexpr Hint<TargetT> copyHintTypesHelper(Hint<T> origin, TargetT target) {
		Hint<TargetT> result = origin;
		result.value = target;	
		return result;
	} 

	//recursive case
	template<template<class> class Hint, IsHintType T,
			class TargetT>
	requires IsHintType<Hint<T>>
	constexpr auto copyHintTypesHelper(Hint<T> origin, TargetT target) {
		Hint<TargetT> result = origin;
		result.value = target;	
		return copyHintTypes(origin.value, result);
	} 

	//defensive no-op
	template<NotHintType T, class TargetT>
	constexpr TargetT copyHintTypes(T origin, TargetT target)
		{ return target; } 

	struct stack_bottom{};

	//
	template<IsHintType T, class TargetT>
	constexpr auto copyHintTypes(T origin, TargetT target) { 
		//copy to a tempory stack, where the and then copy again.
		stack_bottom bottom;		
		auto inverted_hint_types = copyHintTypesHelper(origin, bottom);
		return copyHintTypesHelper(inverted_hint_types, target);
	} 


	//////////////
	//These calculate the return type of `convert(const token& t, const T& fallback)`
	//without actually referencing `convert`

	//these are not real functions, only used for type deduction
	//also these are difficult to extend outside the library
	// you would need to specialize the templates maybe

	//we have complex recursion so we need forward declare
	template<class T> requires NotHintType<T>
	constexpr T
	convert_return_type_f(T a);
	
	template<template<class> class Hint, class T> requires IsHintType<Hint<T>>
	constexpr decltype(convert_return_type_f(std::declval<T>()))
	convert_return_type_f(Hint<T> a);

	template<template<class...> class T, class... Td>
	requires (!MinimalMap<T<Td...>>)
	constexpr T<decltype(convert_return_type_f(std::declval<Td>()))...>
	convert_return_type_f(T<Td...> a);


	//we define the 2 argument template case, so unordered_map doesn't 
	//process the allocator etc 
	template<template<class, class> class T, class T1, class T2>
	requires MinimalMap<T<T1, T2>> 
	constexpr T<
		decltype(convert_return_type_f(std::declval<T1>())), 
		decltype(convert_return_type_f(std::declval<T2>())) 
		>
	convert_return_type_f(T<T1, T2> a);
	
	template<class... Td>
	constexpr std::variant<decltype(convert_return_type_f(std::declval<Td>()))...>
	convert_return_type_f(std::tuple<Td...> a);
		 
	//----- then define them
	template<class T> requires NotHintType<T>
	constexpr T
	convert_return_type_f(T a){return {};};
	
	template<template<class> class Hint, class T> requires IsHintType<Hint<T>>
	constexpr decltype(convert_return_type_f(std::declval<T>()))
	convert_return_type_f(Hint<T> a){return {};};

	template<template<class...> class T, class... Td>
	constexpr T<decltype(convert_return_type_f(std::declval<Td>()))...>
	convert_return_type_f(T<Td...> a){return {};};

	//we define the 2 argument template case, so unordered_map doesn't 
	//process the allocator etc 
	template<template<class, class> class T, class T1, class T2>
	requires MinimalMap<T<T1, T2>> 
	constexpr T<
		decltype(convert_return_type_f(std::declval<T1>())), 
		decltype(convert_return_type_f(std::declval<T2>())) 
		>
	convert_return_type_f(T<T1, T2> a){return {};};

	template<class... Td>
	constexpr std::variant<decltype(convert_return_type_f(std::declval<Td>()))...>
	convert_return_type_f(std::tuple<Td...> a){return {};};

	///-----	 
	template<class T> using convert_return_type = 
		decltype(convert_return_type_f(std::declval<T>()));
	
	//this delays references to convert until later, so library users
	//can extent it
	template<class T> convert_return_type<T>
	convert_delay_define(const token& t, const T& fallback);
	//and this macro defines
#define BOGEYGIRL_GUN_EXTEND_END \
template<class T> \
bogeygirl::gun::convert_return_type<T> \
bogeygirl::gun::convert_delay_define(const token& t, const T& fallback) {\
	return bogeygirl::gun::convert(t, fallback);\
}

	///the real simple conversion

	inline token convert(const token& t, const token& fallback) {
		return t;
	}

	inline std::string convert(const token& t, const std::string& fallback) {
		return std::string(t.text);
	}

	inline std::string_view convert(const token& t, const std::string_view& fallback) {
		return t.text;
	}
	
	inline bool convert(const token& t, const bool& fallback) {
		std::string text = std::string(t.text);
		std::ranges::transform(text, text.begin(),
        [](unsigned char c) { return std::tolower(c); });
		
		std::string_view stripped_text = strlib::strip(text);

		if (stripped_text == "true" || stripped_text == "yes"
			 || stripped_text == "1") return 1;
		if (stripped_text == "false" || stripped_text == "no"
			 || stripped_text == "0") return 0;
		std::println(stderr, "invalid boolean '{}'", t.text);
		return fallback;
	}

	///outsource many types to boost::lexical_conver
//documents if try_lexical_convert can handle these types
//I removed some types that are very unlikely to come up
template<class origin, class target> constexpr inline bool valid_lexical_convert = false;
template<> constexpr inline bool valid_lexical_convert<std::string, signed char> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, unsigned char> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, int> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, short> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, long int> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, long long> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, unsigned int> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, unsigned short> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, unsigned long int> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, unsigned long long> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, float> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, double> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, long double> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, std::array<char, 50>> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, boost::container::string> = true;
template<> constexpr inline bool valid_lexical_convert<std::string, char> = true;
template<> constexpr inline bool valid_lexical_convert<int, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<short, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<long int, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<long long, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned int, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned short, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned long int, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned long long, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<float, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<double, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<long double, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<char*, char> = true;
template<> constexpr inline bool valid_lexical_convert<char*, signed char> = true;
template<> constexpr inline bool valid_lexical_convert<char*, unsigned char> = true;
template<> constexpr inline bool valid_lexical_convert<char*, int> = true;
template<> constexpr inline bool valid_lexical_convert<char*, short> = true;
template<> constexpr inline bool valid_lexical_convert<char*, long int> = true;
template<> constexpr inline bool valid_lexical_convert<char*, long long> = true;
template<> constexpr inline bool valid_lexical_convert<char*, unsigned int> = true;
template<> constexpr inline bool valid_lexical_convert<char*, unsigned short> = true;
template<> constexpr inline bool valid_lexical_convert<char*, unsigned long int> = true;
template<> constexpr inline bool valid_lexical_convert<char*, unsigned long long> = true;
template<> constexpr inline bool valid_lexical_convert<char*, float> = true;
template<> constexpr inline bool valid_lexical_convert<char*, double> = true;
template<> constexpr inline bool valid_lexical_convert<char*, long double> = true;
template<> constexpr inline bool valid_lexical_convert<char*, std::array<char, 50>> = true;
template<> constexpr inline bool valid_lexical_convert<char*, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<char*, boost::container::string> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, char> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, signed char> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, unsigned char> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, int> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, short> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, long int> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, long long> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, unsigned int> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, unsigned short> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, unsigned long int> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, unsigned long long> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, float> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, double> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, long double> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, std::array<char, 50>> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<unsigned char*, boost::container::string> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, char> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, signed char> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, unsigned char> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, int> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, short> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, long int> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, long long> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, unsigned int> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, unsigned short> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, unsigned long int> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, unsigned long long> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, float> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, double> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, long double> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, std::array<char, 50>> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<signed char*, boost::container::string> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, char> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, signed char> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, unsigned char> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, int> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, short> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, long int> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, long long> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, unsigned int> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, unsigned short> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, unsigned long int> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, unsigned long long> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, float> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, double> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, long double> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, std::array<char, 50>> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, std::string> = true;
template<> constexpr inline bool valid_lexical_convert<std::string_view, boost::container::string> = true;
template<> constexpr inline bool valid_lexical_convert<int, int> = true;
template<> constexpr inline bool valid_lexical_convert<float, double> = true;
template<> constexpr inline bool valid_lexical_convert<char, signed char> = true;

	template<class T> concept CanLexicalConvert = valid_lexical_convert<std::string_view, std::remove_const_t<T>>;

	template<class T> requires CanLexicalConvert<T>
	T convert(const token& t, const T& fallback) {
		using boost::conversion::try_lexical_convert;
		T result;	
		if (try_lexical_convert(t.text, result)) {
			return result;
		} else {
			std::println(stderr, "warning: couldn't convert '{}' to '{}' at {}", 
				t.text, typenames::get_name<T>(), t.start_pos);
			return fallback;
		}
	}

	template<class T>
	std::vector<convert_return_type<T> >
	convert(const token& t, const std::vector<T>& fallback) {
		if (fallback.size() != 1) {
			std::println(stderr, "internal error: invalid fallback for '{}'; std::vector fallback must have 1 element", t.text);
			return {};
		}
		std::vector<decltype(convert_delay_define(std::declval<token>(), std::declval<T>()))> result;
		result.reserve(t.children.size());
		for (const token& child : t.children) {
			result.push_back(convert_delay_define(child, fallback.front()));
		}
		return result;
	}

	///no meaning just a unique type value
	class MapPathGuideToken {};

	///should only be used inside a MinimalMap to say where the key and value are.
	template<class T> struct MapPathGuide {
		using isMapPathGuide = MapPathGuideToken; 
		using kind = TypeHintToken; 
		T value;
		token::get_path path;	

		template<class newType> operator
			MapPathGuide<newType>() { return MapPathGuide<newType>{.path = path}; }
		//This seems dubious but it should be fine
		//because there should only ever be one
		//MapPathGuide in a unsorted_set
		bool operator==(MapPathGuide const& rhs) const { return value == rhs.value;};	
	};
	static_assert(IsHintType<MapPathGuide<int>>);
}

//Normally this would be a horrible hash function
//but MapPathGuide barely exists in live data, so it should be fine
template<class T> struct std::hash<bogeygirl::gun::MapPathGuide<T>>{
	std::size_t operator()(const bogeygirl::gun::MapPathGuide<T>& mpg) const
	{
		return std::hash<T>{}(mpg.value); 
	}
};

namespace bogeygirl::gun {

	template<class T> concept isMapPathGuide = std::same_as<typename T::isMapPathGuide, MapPathGuideToken>;
	
	///ignore MapPathGuides if they escape
	///They appear to escape in normal functioning, likely??
	auto convert(const token& t, const isMapPathGuide auto fallback) {
		return convert_delay_define(t, fallback.value);
	}

	template<template<class, class> class Map, class K, class V>
		requires MinimalMap<Map<K,V>>
	Map<convert_return_type<K>, convert_return_type<V>>
		convert(const token& t, const Map<K,V>& fallback) {

		//generate the types for the return value
		//key and value respectively
		using K_r = convert_return_type<K>;
		using V_r = convert_return_type<V>;
		if (fallback.size() != 1) {
			std::println(stderr, "internal error: invalid fallback for '{}'; Map's fallback must have 1 element", t.text);
			return Map<K_r, V_r>{};
		}

		K fallback_k = fallback.begin()->first;
		V fallback_v = fallback.begin()->second;

		token::get_path key_path;
		token::get_path value_path;

		if constexpr (isMapPathGuide<K>) key_path = fallback_k.path;
		else key_path = {0};
		if constexpr (isMapPathGuide<V>) value_path = fallback_v.path;
		else value_path = {1};


		Map<K_r,V_r> result;
		for (const token& tt : t.children) {
			std::optional<token> key_token = tt.get(key_path);
			if (!key_token) {
				std::print(stderr, "warning: '{}' does not have a key at path:", tt.text);
				for (auto i : key_path) std::print(stderr, " {}", i);
				std::println();
				continue;
			}

			std::optional<token> value_token = tt.get(value_path);
			if (!value_token) {
				std::print(stderr, "warning: '{}' does not have a value at path:", tt.text);
				for (auto i : value_path) std::print(stderr, " {}", i);
				std::println();
				continue;
			}

			result[convert_delay_define(*key_token, fallback_k)] = 
					convert_delay_define(*value_token, fallback_v);	
		}
		return result;
	}
	
	///type to describe the preferred conversion policy.
	///This assigns each child in order to field of the struct.
	template<class T> struct LinearStruct {
		using kind = TypeHintToken; 
		T value;

		template<class newType> operator
			LinearStruct<newType>() { return LinearStruct<newType>{}; }
	};
	static_assert(IsHintType<LinearStruct<int>>);

	///type to describe the preferred conversion policy.
	///This assigns each child by key to map-like convert.
	template<class T> struct MapStruct {
		using kind = TypeHintToken; 
		T value;
		template<class newType> operator
			MapStruct<newType>() { return MapStruct<newType>{}; }
	};
	static_assert(IsHintType<MapStruct<int>>);

	template<class T>
	auto convert(const token& t, const LinearStruct<T>& fallback)
	-> stripHintTypes_t<T>
	 {
		namespace pfr = boost::pfr;
		using pfr::get;
		///The type under all the hint types in T
		using T_r = decltype( stripHintTypes( fallback.value ) );
		//our result is type of whatever T will become if we convert it
		// Normally T, but sometimes something else if T is Hint Type
		T_r result;

		if (t.children.size() < pfr::tuple_size_v<T_r>) {
				std::println(stderr,
					"warning: ran out of fields to populate, have {}, need {}; at {}", 
					t.children.size(), pfr::tuple_size_v<T_r>, t.start_pos);
		}
		if (t.children.size() > pfr::tuple_size_v<T_r>) {
				std::println(stderr,
					"warning: too many fields to populate, have {}, only need {}; at {}", 
					t.children.size(), pfr::tuple_size_v<T_r>, t.start_pos);
		}
		///this is the fallback value under all the Hint Types, including ourselves
		T_r true_fallback = stripHintTypes( fallback.value );

		pfr::for_each_field(result, 
					[&t, &fallback, &true_fallback, &result](const auto& _, auto idxx) {
			constexpr size_t idx = idxx.value;
			auto fallback_f = get<idxx>(true_fallback);
		
			if (idx < t.children.size()) {
				//because T is a hint type, we should reapply the stack 
				auto hinted_fallback_f = copyHintTypes(fallback.value, fallback_f);
				get<idx>(result) = convert_delay_define(t.children[idx], hinted_fallback_f);
			} else {
				std::println(stderr, "\tout of values for {}; default to {} ",
					pfr::get_name<idxx, T_r>(), fallback_f);
				get<idx>(result) = fallback_f;
			}
		});

		return result;
	}

	template<class T> 
	auto convert(const token& t, const MapStruct<T>& fallback)
	//I worry this is wrong because what
	//if there are HintTypes inside the nearest normal type
	->	decltype( stripHintTypes( fallback.value ) )
	 {
		std::unordered_map<std::string_view, token> map = {{"", {}}};
		decltype(map) first_pass = convert_delay_define(t, map); 

		using T_r = decltype( stripHintTypes( fallback.value ) );
		namespace pfr = boost::pfr;
		using pfr::get;
		T_r result;
		const T_r& true_fallback = stripHintTypes( fallback.value );

		pfr::for_each_field_with_name(result, 
			[&true_fallback, &fallback, &result, &first_pass](
				std::string_view field_name, const auto& _, auto idxx) {
				constexpr size_t idx = idxx.value;
				auto fallback_f = get<idx>(true_fallback);
			
				if (first_pass.contains(field_name)) {
					auto hinted_fallback_f = copyHintTypes(fallback.value, fallback_f);
					get<idx>(result) = convert_delay_define(first_pass.at(field_name), hinted_fallback_f);
				} else {
					get<idx>(result) = fallback_f;	
				}
		});

		return result;
	}

	///types to re-arrange tokens for conversion

	///Unnest. removes one extra layer of layering in token.
	template<class T> struct Unnest {
		using kind = TypeHintToken; 
		T value;
		template<class newType> operator
			Unnest<newType>() { return Unnest<newType>{}; }
	};
	static_assert(IsHintType<Unnest<int>>);

	template<class T>
	auto convert(const token& t, const Unnest<T>& fallback) {
		if (t.children.empty()) {
			return convert_delay_define(t, fallback.value);
		} else if (t.children.size() > 1) {
			std::println(stderr,
			  "Warning: token from {} to {} has more than 1 child, namely {}, using first one",
				t.start_pos, t.end_pos, t.children.size());
		}
		return convert_delay_define(t.children[0], fallback.value);
	}

	///applies the rest of the types on the nth child of the token
	template<class T> struct NthChild {
		using kind = TypeHintToken; 
		size_t child_index;
		T value;
		template<class newType> operator
			NthChild<newType>() { return NthChild<newType>{.child_index = child_index}; }
	};
	static_assert(IsHintType<NthChild<int>>);

	template<class T>
	convert_return_type<T> convert(const token& t, const NthChild<T>& fallback) {
		if (t.children.empty()) {
			std::println(stderr,
			  "Warning: token from {} to {} no children. ignoring NthChild",
				t.start_pos, t.end_pos);
			return convert_delay_define(t, fallback.value);

		} else if (t.children.size() <= fallback.child_index) {
			std::println(stderr,
			  "Warning: token from {} to {} less than {} children, namely {}, using last one",
				t.start_pos, t.end_pos, fallback.child_index+1, t.children.size());
			return convert_delay_define(t.children.back(), fallback.value);
		} else {
			return convert_delay_define(t.children[fallback.child_index], fallback.value);
		}
	}

	///gets the ith element of a tuple
	template<size_t I=0, class... T>
	std::variant<T...> get_ith(size_t i, std::tuple<T...> tup) {
		if constexpr (I >= sizeof...(T)) {
			throw std::out_of_range(
				std::format("argument {} to big for tuple of length {}", i, sizeof...(T)));
		} else {
			if (i == I) return std::get<I>(tup);	
			return get_ith<I+1, T...>(i, tup);
		}
	}

	template<class... T>
	convert_return_type<std::tuple<T...>>
	convert(const token& t, const std::tuple<T...>& fallback) {
		if (t.alternative >= sizeof...(T)) {
			throw std::out_of_range(std::format("the parse alternative ({}) is too big for target type {}", t.alternative, typenames::get_name<decltype(fallback)>()));
		}


		auto result = get_ith(t.alternative, fallback);
		return std::visit(
			[t](auto f) -> std::variant<convert_return_type<T>...>
				{return convert_delay_define(t, f);}, 
			result);
	}

	/// inheretence
	template<class T> struct ReplaceFallback {
		using kind = TypeHintToken; 
		T value;
		std::unordered_map<std::string_view, stripHintTypes_t<T>> const * map;
		token::get_path path;
		template<class newType> operator ReplaceFallback<newType>() {
			return ReplaceFallback<newType>{.map = map, .path = path};
		}
	};
	static_assert(IsHintType<ReplaceFallback<int>>);

	template<class T>
	auto convert(const token& t, const ReplaceFallback<T>& fallback) {
		auto key_token = t.get(fallback.path);
		if (!key_token) {
			std::println(stderr, "Warning: no token found at path: {}", fallback.path); 
			goto error_recovery;
		}

		//silently ignore an empty parent, that is no parent.
		if (key_token->text.empty()) goto error_recovery;
		
		if (!fallback.map->contains(key_token->text)) {
			std::println(stderr, "Warning: '{}' key not found", key_token->text);
			goto error_recovery;
		}
		
		{
		auto new_fallback = copyHintTypes(fallback.value, fallback.map->at(key_token->text));
		return convert_delay_define(t, new_fallback);
		}

		std::unreachable(); //<---------- !!!

		error_recovery: return convert_delay_define(t, fallback.value);
	}

	///fill a map structure with names and values
	///Designed to use a struct as the backing data type
  ///maybe it works with other ones?
	template<class T> struct FillMap {
		using kind = TypeHintToken; 
		T value;
		std::unordered_map<std::string_view, stripHintTypes_t<T>> * map;
		token::get_path name_path;
		template<class newType> operator FillMap<newType>() {
			return FillMap<newType>{.map = map, .name_path = name_path};
		}
	};
	static_assert(IsHintType<FillMap<int>>);
	
	template<class T>
	auto convert(const token& t, const FillMap<T>& fallback) {
		auto result = convert_delay_define(t, fallback.value);

		std::optional<token> key_token = t.get(fallback.name_path);
		if (!key_token) {
			std::print(stderr, "Warning: no token found at path: {}", fallback.name_path); 
			return result;	
		} else {
			std::string_view name = key_token->text;
			(*fallback.map)[name] = result; 
			return result;
		}
	}

	///Call a function and return the result as the conversion
	template<class T> struct UseFunc {
		using kind = TypeHintToken; 
		T value;
		std::function<T(const token&)> func;
		template<class newType> operator UseFunc<newType>() {
			return UseFunc<newType>{.func = func};
		}
		bool operator==( const UseFunc& rhs) const { 
			return value == rhs.value; 
			//unfornately std::function doesn't seem to be comparable
			//so we can't compare those?
		};	
	};
	static_assert(IsHintType<UseFunc<int>>);
}

//delegate hashing to the inner type
template<class T> struct std::hash<bogeygirl::gun::UseFunc<T>>{
	std::size_t operator()(const bogeygirl::gun::UseFunc<T>& x) const
	{
		return std::hash<T>{}(x.value); 
	}
};

namespace bogeygirl::gun {	
	///Call a function and return the result as the conversion
	template<class T>
	auto convert(const token& t, const UseFunc<T>& fallback) {
		return fallback.func(t);	
	}

	///this Hint type purposely ships with no convert function
	///so you can easily define your own
	template<class T> struct AltConvert {
		using kind = TypeHintToken; 
		T value;
		bool operator==(AltConvert<T> const& rhs) const { return value == rhs.value;};	
	};
}

//delegate hashing to the inner type
template<class T> struct std::hash<bogeygirl::gun::AltConvert<T>>{
	std::size_t operator()(const bogeygirl::gun::AltConvert<T>& alt) const
	{
		return std::hash<T>{}(alt.value); 
	}
};

namespace bogeygirl::gun {
	

	///returns the prase_fallback if the parse fails
	///returns fallback if the inner convert fails
	template<class T, class R, class S> S
		reparse_convert(const rule<R> syntax, 
		const token& t, const T& fallback, const S& prase_fallback)
				requires requires (token t) {
					{convert_delay_define(t, fallback)} -> std::same_as<S>;
				}
	{
		std::optional<bogeygirl::token> parse_result
			= bogeygirl::prase(syntax, bogeygirl::whitespace_skip, t.text, t.start_pos);
		if (parse_result) {
			return convert_delay_define(*parse_result, fallback);
		} else {
			std::println("warning: failed to parse '{}' at {}",
				typenames::get_name<T>(), t.start_pos);
			return prase_fallback;
		}
	}

	///Tool for easily defining a convert where you parse the text in
	///a token, and then coerce it into a given type.
	///Warning! it only works with stripable helper types.
	template<class T, class R> convert_return_type<T>
		reparse_convert(const rule<R>& syntax, 
		const token& t, const T& fallback) {

		using true_T = stripHintTypes_t<T>;
		static_assert(std::same_as<true_T,convert_return_type<T>>,
			"Merely stripping the helper types around T"
       "needs to be enough to find return type"
			"use the 4 paramater form for complex cases"
			);
		stripHintTypes_t<T> fallback_value = stripHintTypes(fallback);

		return reparse_convert(syntax, t, fallback, fallback_value);
	}

	
	/////////////////testing

	//test stripHintTypes
	static_assert(std::same_as<decltype(stripHintTypes(std::declval<LinearStruct<int>>())),int>);
	
	//test copyHintTypes
	static_assert(std::same_as<
	   decltype(copyHintTypes(Unnest<LinearStruct<int>>{}, float{0}))
   , Unnest<LinearStruct<float>>>);

	//test the mapPathGuide
	namespace {
		struct foo {int a; float b;};
		using fallback = std::unordered_map<MapPathGuide<int>, std::vector<std::tuple<Unnest<int>, MapStruct<foo>>>>;
		using result = std::unordered_map<int, std::vector<std::variant<int, foo>>>;
		using result_real = convert_return_type<fallback>;
		
		static_assert(std::same_as<result, result_real>);
	}
	
}

// vim: ts=2 sw=2
