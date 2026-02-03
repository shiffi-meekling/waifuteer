#include "math_util.h"
#include "./libs/doctest.h"

TEST_CASE("math util rescale") {
	CHECK(rescale(0.5, 0., 1.) == 0.5);
	CHECK(rescale(1., 0., 5.) == 0.2);
	CHECK(rescale(9.5, 9., 10.) == 0.5);
	CHECK(rescale(-10., -100., 0.) == 0.9);
};

TEST_CASE("math util recale out of bounds") {
	CHECK(rescale(2., 0., 1.) == 2.);
	CHECK(rescale(10., 0., 5.) == 2);
	CHECK(rescale(8., 9., 10.) == -1);
	CHECK(rescale(-101., -100., 0.) == -0.01);
};

TEST_CASE("linear_interpolate") {
	CHECK(linear_interpolate(0, 5, 99, 99) == 5);
	CHECK(linear_interpolate(1.5, -10.0, 2, 2) == -10.0);
	CHECK(linear_interpolate(10, 20, 3, 3) == 20);
	CHECK(linear_interpolate(10.0, 20.0, 3, 3) == 20);

}

// vim: ts=2 sw=2
