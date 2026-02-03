#include "bogeygirl.hpp"
#include "gun.hpp"

#include <fstream>
#include <string>
#include <filesystem>
#include <print>
#include <string_view>

constexpr auto parse_format() {
		using namespace bogeygirl;
		rule range = lit("[") > word > lit(",") > word > lit("]");
		rule pair = rex("[0-9]+") >> lit(":") >> word >> lit(":") >> range >> newln;

		rule document = lit("# numbers") >> newln >> *pair; 
		
		return document;
}

auto convert_to_data(const bogeygirl::token& t) {
	using namespace bogeygirl::gun;

	auto a = std::unordered_map<MapPathGuide<std::string>, MapPathGuide<int>>{{{"hello", {1}}, {12, {0}}}};
	static_assert(MinimalMap<decltype(a)>);
	
	return convert(t, Unnest<std::unordered_map<MapPathGuide<std::string>, MapPathGuide<int>>>{{{{"hello",{2,1}}, {12, {0}} }}});
}

std::string parsed_filename = "test-strange-map.txt";
int main() {
	std::ifstream file(parsed_filename);
	std::string content{std::istreambuf_iterator<char>(file),
			 std::istreambuf_iterator<char>()};

	std::optional<bogeygirl::token> parse_result
		= bogeygirl::prase(parse_format(), bogeygirl::space_skip, content);

	if (parse_result) {
		std::println("parse successful!");
		std::println("{} children", parse_result->children.size());

		bogeygirl::pretty_print(*parse_result);

		std::println("index: {}-{}", parse_result->start_pos.index, parse_result->end_pos.index);

		auto result = convert_to_data(*parse_result);
	
		for (auto [key, value] : result) {
			std::println("{} : {}", key, value);
		}	
		
	}
}
// vim: ts=2 sw=2
