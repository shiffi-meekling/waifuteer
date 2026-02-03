#include "../libs/doctest.h"
#include "bogeygirl.hpp"
#include "gun.hpp"


struct optionA {
	std::string a;
};

struct optionB {
	std::string b;
};

namespace bogeygirl::gun {
	optionA convert(const bogeygirl::token& t, const optionA& fallback) {
		return {std::string(t.text)};
	}
	optionB convert(const bogeygirl::token& t, const optionB& fallback) {
		return {std::string(t.text)};
	}
}

BOGEYGIRL_GUN_EXTEND_END;
using namespace bogeygirl;
using namespace gun;

TEST_CASE("gun - alternatives") {
	rule a = lit(":") >> word;
	rule b = lit("=") >> word;
	SUBCASE("simple") {
		rule c = squeeze(a || b);
		std::string text = "= bla";
		std::optional<token> result = prase(c,space_skip,text);
		REQUIRE(result.has_value());
		token t = *result;
		REQUIRE(t.alternative == 1);
		auto converted = convert(t, std::tuple<optionA, optionB>{});
		REQUIRE(std::holds_alternative<optionB>(converted));
		CHECK(std::get<optionB>(converted).b == "bla");
	}
}

TEST_CASE("gun - bool with spaces") {
	token t = {"yes   ", {}, {0,0,0}, {7,0,7}, 0};
	CHECK(convert(t, false) == true);
}
// vim: ts=2 sw=2
