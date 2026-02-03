#include "libs/doctest.h"
#include "ndim_knn_map.h"
#include <print>
#include <vector>
#include <string>

TEST_CASE("l1_distance") {
	CHECK(l1_distance(std::vector{0.0f}, std::vector{1.0f}) == 1);
	CHECK(l1_distance(std::vector{0.0f, 0.0f}, std::vector{1.0f, 1.0f}) == 2);
	CHECK(l1_distance(std::vector{0.0f, 0.0f, 0.0f}, std::vector{0.0f, 0.0f, 0.0f}) == 0);
}

TEST_CASE("ndim_knn_map") {
	ndim_knn_map<std::string> a(2);
	a.push_back({1,1}, "hehe");
	a.push_back({0,0}, "meow");
	REQUIRE(a.size() == 2);
	REQUIRE(a.dimentions() == 2);
	SUBCASE("get") {
		CHECK(a.get({0.1f, 0.0}) == "meow");
		CHECK(a.get({1.1f, 0.5}) == "hehe");
	}
	SUBCASE("push_back changes results") {
		CHECK(a.get({0.0f, 0.9}) == "meow");
		a.push_back({0,1}, "nyan");
		CHECK(a.get({0.0f, 0.9}) == "nyan");
		REQUIRE(a.size() == 3);
	}
	SUBCASE("dimentions changed by assignment") {
		a = ndim_knn_map<std::string>(1);
		CHECK(a.dimentions() == 1);
		CHECK(a.size() == 0);
	}
	SUBCASE("assignment by initializer_list") {
		a = ndim_knn_map<std::string>({ {{0,0,0}, "center"} });
		REQUIRE(a.dimentions() == 3);
		CHECK(a.size() == 1);
		CHECK(a.get({-100, -100, -100}) == "center");
		CHECK(a.get({100, -100, -100}) == "center");
	}
	SUBCASE("iterator") {
		CHECK(*(a.begin()) == ndim_knn_map<std::string>::value_type{{1,1}, "hehe"});
	}

	SUBCASE("range-for loop") {
		size_t i = 0;
		for (auto [k,v] : a) {
			switch (i) {
				case 0: 
					CHECK(k == ndim_knn_map<std::string>::key_type{1,1});
					CHECK(v == "hehe");
					break;
				case 1: 
					CHECK(k == ndim_knn_map<std::string>::key_type{0,0});
					CHECK(v == "meow");
					break;
			}
			++i;
		}
	}
}

TEST_CASE("ndim_knn_map") {
	ndim_knn_map<int> a(2);
	a.push_back({0,0}, 1);
	a.push_back({0,1}, 2);
	a.push_back({1,0}, 3);
	a.push_back({1,1}, 4);
	CHECK(a.get({-1, -1}) == 1);
	CHECK(a.get({-0.04, 0.9}) == 2);
	CHECK(a.get({0.04,  0.9}) == 2);
	CHECK(a.get({0.04,  1.1}) == 2);
	CHECK(a.get({0.8, 0.05})  == 3);
	CHECK(a.get({0.54, 0.4})  == 3);
	CHECK(a.get({0.51, 0.49}) == 3);
	CHECK(a.get({1.1, 0.49})  == 3);
	CHECK(a.get({0.52, 0.51}) == 4);
	CHECK(a.get({0.9, 0.8})   == 4);
	CHECK(a.get({100, 10})    == 4);
}
// vim: ts=2 sw=2
