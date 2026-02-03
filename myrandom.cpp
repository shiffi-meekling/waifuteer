#include <random>

auto seed = 5;
std::mt19937 gen(seed); //Standard mersenne_twister_engine seeded with rd()

int randint(int a, int b) {
	std::uniform_int_distribution<> distrib(a, b);
	return distrib(gen);
}

// vi: ts=2 sw=2
