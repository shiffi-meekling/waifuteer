#include "bogeygirl.hpp"
#include "gun.hpp"

#include <fstream>
#include <string>
#include <filesystem>
#include <print>
#include <tuple>
#include <unordered_map>

struct weer { int i; float f;};

std::string parsed_filename = "test-key-pair.txt";
int main() {
	std::ifstream file(parsed_filename);
	std::string content{std::istreambuf_iterator<char>(file),
			 std::istreambuf_iterator<char>()};

	bogeygirl::rule number = bogeygirl::rex("[+-]?[0-9]+(\\.[0-9]*)?");
	bogeygirl::rule solo_number = number > bogeygirl::lit("\n");

	bogeygirl::rule pair = bogeygirl::word > bogeygirl::lit(",") > number > bogeygirl::lit("\n");

	bogeygirl::rule header = bogeygirl::lit("##") > bogeygirl::word
	 > bogeygirl::lit("#") > bogeygirl::word > bogeygirl::lit("\n");

	bogeygirl::rule statement = header
		> +pair > bogeygirl::lit("\n");
	
	bogeygirl::rule statements = +(statement || solo_number);

	std::optional<bogeygirl::token> foo
		= bogeygirl::prase(statements, bogeygirl::space_skip, content);

	if (foo) {
		std::println("parse successful!");
		std::println("{} children", foo->children.size());

		bogeygirl::pretty_print(*foo);

		std::println("index: {}-{}", foo->start_pos.index, foo->end_pos.index);

		namespace g = bogeygirl::gun;
		
		std::unordered_map<std::string_view, weer> parents;
		auto fallback = std::vector {

			std::tuple {

				g::FillMap{
					g::ReplaceFallback{
						g::NthChild{1,
							g::MapStruct{
								weer{89, 2.0}
							},
						},
					&parents, {0,1}},
				&parents, {0,0} }
			,
				int{}
			}

		};
		
		auto converstion = g::convert(*foo, fallback);
		
		for (auto j : converstion) {
			if (std::holds_alternative<weer>(j)) {
				auto i = std::get<weer>(j);
				std::println("converted: {} {}", i.i, i.f);
			} else {
				auto i = std::get<int>(j);
				std::println("int : {}", i);
			}
		}
		
	}
}
// vim: ts=2 sw=2
