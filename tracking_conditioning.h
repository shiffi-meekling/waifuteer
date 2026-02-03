#pragma once
#include "vec.h"
#include <filesystem>

namespace tracking::conditioning {

void init(const std::filesystem::path& config_filename) ;
///extra-wiggles for metrics which are not relative
vec2 absolute_excite(vec2);
uint16_t port();

#define X_SCALE(name) float name##_scale(float a);

X_SCALE(mouth_open);
X_SCALE(mouth_wide);

#undef X_SCALE
}
// vim: ts=2 sw=2
