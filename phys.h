#pragma once
#include "vec.h"

struct phys {
	vec2 p; ///position
	vec2 v; ///velocity 

	///resets to 0,0 vel and 0,0 pos if either is nan
	void haps_reset();
	void tied_to(float pull_length, vec2 target, float power);
	void tied_to(float pull_length, vec2 target, float power, float max_length);
	void push_to(float push_length, vec2 target, float power);
	void push_to(float push_length, vec2 target, float power, float min_length);
	void elbow_ik(float length, vec2 shoulder, vec2 hand);

	void play() {
		p += v;
	}

	//move according another phys object.
	void play(phys other) {
		p += other.v;
	}

	//wind (like rewind) but there is no 're' since we are going somewhere new
	void wind(vec2 new_pos) {
		v = new_pos - p;
		p = new_pos;
	}
	
	//confidence varries from 0 to 1
	//at confidence=0 this is equivalent to play
	//at confidence=1 this is equivalent to wind
	void influence(vec2 new_pos, float confindence, float stillness = 0) {
		vec2 old_pos = p;
		vec2 coasting_p = (p+v).towards(stillness, p);

		p = coasting_p*(1.0-confindence) + new_pos*confindence;
		v = v.towards(confindence, p - old_pos);
	}
};

#include <ostream>
std::ostream& operator<<(std::ostream& out, phys rhs) ;

#include <format>
template <>
struct std::formatter<phys> {
	constexpr auto parse(std::format_parse_context& ctx) {
			return ctx.begin();
	}

	auto format(const phys& data, std::format_context& ctx) const {
			auto result = ctx.out();
			result = std::format_to(result, "{{{},{}}}", data.p, data.v);
			return result;
	}
};
// vim: ts=2 sw=2
