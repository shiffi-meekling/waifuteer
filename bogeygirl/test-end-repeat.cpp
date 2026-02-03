#include "bogeygirl.hpp"
#include "gun.hpp"

#include <fstream>
#include <string>
#include <filesystem>
#include <print>
#include <string_view>

struct name {
	std::string foo;
	std::vector<int> numbs;
};

constexpr auto section_format(const std::string_view text) {
	bogeygirl::rule header = bogeygirl::lit("===") >> bogeygirl::lit(text)
	 >> bogeygirl::lit("===") >> bogeygirl::lit("\n");
	return header; 
}

constexpr auto parse_format() {
		using namespace bogeygirl;
		flattening_rule pair = rex("[0-9]+") >> by_skip(whitespace_skip);
		
		rule image = +pair;
		rule images = section_format("images") >> *image;
		
		rule motte = +pair;
		rule mottes = section_format("mottes") >> *motte;

		rule document = by_skip(whitespace_skip) >>
			images >>
			mottes;
		
		return document;
}

std::string parsed_filename = "test-end-repeat.txt";
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

	}
}
// vim: ts=2 sw=2
