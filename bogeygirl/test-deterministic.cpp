#include "bogeygirl.hpp"
#include "gun.hpp"

#include <fstream>
#include <string>
#include <filesystem>
#include <print>
#include <string_view>

constexpr auto parse_format() {
		using namespace bogeygirl;
		rule term = word > lit(":") > word > lit(";");
		rule document = lit("head:") >> word >> lit(";") >> *term;
		return document;
}

std::string parsed_filename = "test-deterministic.txt";
int main() {
	std::ifstream file(parsed_filename);
	std::string content{std::istreambuf_iterator<char>(file),
			 std::istreambuf_iterator<char>()};


	std::optional<bogeygirl::token> parse_result
		= bogeygirl::prase(parse_format(), bogeygirl::whitespace_skip, content);

	if (parse_result) {
		std::println("parse successful!");
		std::println("{} children", parse_result->children.size());

		bogeygirl::pretty_print(*parse_result);

		std::println("index: {}-{}", parse_result->start_pos.index, parse_result->end_pos.index);

	}
}
// vim: ts=2 sw=2
