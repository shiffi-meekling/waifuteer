#include "libs/doctest.h"
#include "parsing_bits.h"
#include "puppet-convert.h"
BOGEYGIRL_GUN_EXTEND_END

using namespace bogeygirl;
using namespace gun;

TEST_CASE("puppet-convert - vec2") {
	rule a = rex("[^#]+");
	rule b = a % lit("#");
	std::string text = "2.3,1.1#0.5,0.1";
	std::optional<token> result = prase(b,space_skip,text);
	REQUIRE(result.has_value());
	token t = *result;
	REQUIRE(t.children.size() == 2);
	std::vector<vec2> converted = gun::convert(t, std::vector<vec2>{{0,0}});
	CHECK(converted.at(0) == vec2{2.3, 1.1});
}

// vim: ts=2 sw=2
