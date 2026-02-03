#include "../libs/doctest.h"
#include "strlib.hpp"
#include <string_view>
#include <string>

TEST_CASE("strlib::strip - idenity") {
	std::string_view alice = "alice";
	CHECK(strlib::strip(alice) == alice);	
}

TEST_CASE("strlib::strip - spaces around") {
	std::string_view alice = " spaces around  ";
	CHECK(strlib::strip(alice) == "spaces around");	
}

TEST_CASE("strlib::strip - ops all spaces") {
	std::string_view alice = "   ";
	CHECK(strlib::strip(alice) == "");	
}

TEST_CASE("strlib::replace") {
	std::string a = "aaAAAf";
	strlib::replace(a, 'A', '_');
	CHECK("aa___f" == a);
}

TEST_CASE("strlib::lower") {
	std::string a = "aa_AAAf";
	strlib::lower(a);
	CHECK("aa_aaaf" == a);
}

TEST_CASE("strlib::upper") {
	std::string a = "aa_AAAf";
	strlib::upper(a);
	CHECK("AA_AAAF" == a);
}
// vim: sw=2 ts=2
