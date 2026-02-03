#include <filesystem>
#include <print>
#include <fstream>
#include <string>
#include "bogeygirl/bogeygirl.hpp"
#include "bogeygirl/gun.hpp"
#include "puppet-convert.h"
BOGEYGIRL_GUN_EXTEND_END //we are done overloading bogeygirl::gun::convert now

#include "parsing_bits.h"
#include "math_util.h"
#include "tracking_conditioning.h"
#include "filelayer.h"

namespace tracking::conditioning {


#define X_SCALE_CONFIG(name) vec2 name##_scale = {0.0, 1.0};


struct config {
	vec2 absolute_excite;
	vec2 absolute_offset;
	float easing;
	//we abuse vec2 to stand for minimum and maximum to scale from
	X_SCALE_CONFIG(mouth_open);
	X_SCALE_CONFIG(mouth_wide);
	uint16_t port;
};

static config cfg = {{1,1}, {0,0}, 0.5, {0,1}, {0,1}, 11573};

void init(const std::filesystem::path& path) {
	const std::filesystem::path config_filename = filelayer::canonise_path(path);
	// need newln_or_c in case the first line is a comment
	const bogeygirl::rule document_syntax = -parsing_bits::newln_or_c >> (*parsing_bits::pair);

	std::ifstream file(config_filename);
	if (file.fail()) {
		std::println(stderr, "could not find the tracking config file: {}", std::string(config_filename));
		std::println(stderr, "using defaults.");
		return;
	}
	std::string content{std::istreambuf_iterator<char>(file),
			 std::istreambuf_iterator<char>()};

	//try parsing the file
	std::optional<bogeygirl::token> maybe_parse_tree
		= bogeygirl::prase(document_syntax, bogeygirl::space_skip, content);
	if (!maybe_parse_tree) {
		std::println(stderr, "using defaults, since parse failed.");
		return; 
	}
	
	//try the fields into the type
	namespace gun = bogeygirl::gun;
	cfg = gun::convert(maybe_parse_tree->children.at(0), gun::MapStruct<config>(cfg));
}

vec2 absolute_excite(vec2 a) {
	return (cfg.absolute_excite * a) + cfg.absolute_offset;
}

uint16_t port() {
	return cfg.port;
}

#define X_SCALE(name) \
float name##_scale(float a) {return rescale(a, cfg. name##_scale.x, cfg. name##_scale.y);}

X_SCALE(mouth_open);
X_SCALE(mouth_wide);

}
// vim: ts=2 sw=2
