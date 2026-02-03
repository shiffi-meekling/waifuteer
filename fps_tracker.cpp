#include <chrono>

namespace c = std::chrono;
double get_fps() {
	static std::chrono::high_resolution_clock clock;
	static auto time = clock.now();
	double fps = 1000.0/(c::duration<double, std::milli>((clock.now() - time)).count());
	time = clock.now();
	return fps;
}

double get_fps(double& storage) {
	std::chrono::time_point<c::high_resolution_clock, c::duration<double, std::milli> >
		current_time = std::chrono::high_resolution_clock::now();
	double c_time = current_time.time_since_epoch().count();
	double fps = 1000.0 / (c_time - storage);
	storage = c_time; 
	return fps;
}

// vim: sw=2 ts=2
