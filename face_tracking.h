#pragma once
#include "vec.h"
#include <optional>
#include <format>

namespace tracking {

///this is the tracking information we gather from the camera
struct metrics {
	vec2 nose; //graphics-space location of nose
	vec2 head_up_unit_vector;
//	float left_eye_squint;
//	float right_eye_squint;
//	float left_eye_brow;
//	float right_eye_brow;
	float mouth_open;
	float mouth_wide;
};

void init();
///get a new frame from the camera
void new_frame();
std::optional<metrics> get_metrics();

//see also face_tracking_debug.h for more highly integrated types 
}

template<>
struct std::formatter<tracking::metrics> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const tracking::metrics& m, std::format_context& ctx) const {
        return std::format_to(ctx.out(), 
				"nose: {}\n"
//				"left_eye_squint: {}\n"
//				"right_eye_squint: {}\n"
//				"left_eye_brow: {}\n"
//				"right_eye_brow: {}\n"
				"mouth_open: {}\n"
				"mouth_wide: {}\n"
				, m.nose 
			//	, m.left_eye_squint, m.right_eye_squint
			//	, m.left_eye_brow, m.right_eye_brow
				, m.mouth_open, m.mouth_wide
				);
    }
};

// vim: ts=2 sw=2
