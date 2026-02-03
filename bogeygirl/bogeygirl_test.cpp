#include "../libs/doctest.h"
#include "bogeygirl.hpp"

using namespace bogeygirl;

TEST_CASE("bogeygirl - is_flat") {
	std::string text = "abcabc";
	token flat_t = {text, {{text, {}}, {text, {}}}};
	CHECK(flat_t.is_flat());
	token no_flat_t = {text, { {text,{{text,{}}}}, {text, {}} } };
	CHECK_FALSE(no_flat_t.is_flat());
}

TEST_CASE("bogeygirl - flatten token") {
	rule a = lit("{") >> word >> *(lit(",") >> word) >> lit("}");
	rule b = a >> *(lit(",") >> a);
	std::string text = "{aaa}, {a,b}, {c}";
	std::optional<token> result = prase(b,space_skip,text);
	REQUIRE(result.has_value());
	SUBCASE("flatten token") {
		token t = *result;
		token tt = flatten_token(t);
		CHECK(t.text == tt.text);
		CHECK(t.alternative == tt.alternative);
		CHECK(t.end_pos == tt.end_pos);
		CHECK(t.start_pos == tt.start_pos);
		CHECK(t.assert_shape());
		CHECK(tt.assert_shape());

		///children should be empty of the flattened child
		CHECK(tt.is_flat());	

		REQUIRE(tt.children.size() == 4);
		CHECK(tt.children[0].text == "aaa");
		CHECK(tt.children[1].text == "a");
		CHECK(tt.children[2].text == "b");
		CHECK(tt.children[3].text == "c");
	}	
}

TEST_CASE("bogeygirl - flatten") {
	rule a = lit("{") >> word >> *(lit(",") >> word) >> lit("}");
	rule b = flatten(a >> *(lit(",") >> a));
	std::string text = "{aaa}, {a,b}, {c}";
	std::optional<token> result = prase(b,space_skip,text);
	REQUIRE(result.has_value());

	token t = *result;
	CHECK(t.is_flat());	
}

TEST_CASE("bogeygirl \% basic") {
	std::string text = "ab, aaaa, wert";
	rule the_rule = word % lit(",");
	std::optional<token> result = prase(the_rule, space_skip, text);
	REQUIRE(result.has_value());
	
	std::vector<std::string> correct_result = {"ab", "aaaa", "wert"};
	REQUIRE(result->children.size() == correct_result.size());
	for (int i =0; i < result->children.size(); ++i) {
		CHECK(result->children[i].text == correct_result[i]);
	}
}

TEST_CASE("bogeygirl unnest") {
	std::string text = "{{a}}";
	rule a = lit("{") >> word >> lit("}");
	rule the_rule = lit("{") >> a >> lit("}");
	std::optional<token> result = prase(the_rule, space_skip, text);
	REQUIRE(result.has_value());
	token t = *result;
	CHECK_NOTHROW(std::ignore = t.children.at(0).children.at(0));

	rule fixed_rule = unnest(unnest(the_rule));
	std::optional<token> fix_result = prase(fixed_rule, space_skip, text);
	REQUIRE(fix_result.has_value());
	token tt = *fix_result;
	CHECK(tt.assert_shape());
	CHECK(tt.children.size() == 0);
	CHECK(tt.text == "a");
}

TEST_CASE("bogeygirl \% nested") {
	std::string text = "{aaa bbb} % {aa wer oioiu} % {lksjdlfkjsdl}";
	flattening_rule a = lit("{") >> *word >> lit("}");
	rule the_rule = a % lit("%");
	std::optional<token> result = prase(the_rule, space_skip, text);
	REQUIRE(result.has_value());
	
	std::vector<int> correct_size = {2, 3, 1};
	REQUIRE(result->children.size() == correct_size.size());
	for (int i =0; i < result->children.size(); ++i) {
		CHECK(result->children[i].children.size() == correct_size[i]);
	}
}

TEST_CASE("bogeygirl - alternative") {
	rule a = lit(":") >> word;
	rule b = lit("=") >> word;
	SUBCASE("simple") {
		rule c = a || b;
		std::string text = "= bla";
		std::optional<token> result = prase(c,space_skip,text);
		REQUIRE(result.has_value());
		token t = *result;
		CHECK(t.alternative == 1);
		CHECK(t.children.front().alternative == 0);
		
		SUBCASE("squeezed") {
			rule cc = squeeze(c);
			std::optional<token> result = prase(cc,space_skip,text);
			REQUIRE(result.has_value());
			token t = *result;
			CHECK(t.alternative == 1);
		}
		SUBCASE("copy alternative down") {
			rule cc = copy_alt_down(c);
			std::optional<token> result = prase(cc,space_skip,text);
			REQUIRE(result.has_value());
			token t = *result;
			CHECK(t.children.front().alternative == 1);
		}
	}

	SUBCASE("complex") {
		rule c = squeeze(*(a || b));

		std::string text = ": foo : bar = bla";
		std::optional<token> result = prase(c,space_skip,text);
		REQUIRE(result.has_value());


		token t = *result;
		REQUIRE(t.is_flat());
		REQUIRE(t.children.size() == 3);	
		CHECK(t.children[0].alternative == 0);
		CHECK(t.children[1].alternative == 0);
		CHECK(t.children[2].alternative == 1);
	
	}
}



// vim: ts=2 sw=2
