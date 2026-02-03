#pragma once
#include "./bogeygirl/bogeygirl.hpp"
#include "./bogeygirl/gun.hpp"
#include "vec.h"
#include <algorithm>
#include <cstdlib>
#include <print>
#include "puppet.h"
#include "textures.h"
#include <ranges>
#include <string_view>
#include "myGl.h"
#include "parsing_bits.h"

namespace bogeygirl::gun {

	vec3 inline convert(const token& t, const vec3& fallback) {
		const rule syntax = float_ > lit(",") > float_ > lit(",") > float_;
		return reparse_convert(syntax, t, LinearStruct<vec3>(fallback));
	}

	vec2 inline convert(const token& t, const vec2& fallback) {
		const rule syntax = float_ > lit(",") > float_;
		return reparse_convert(syntax, t, LinearStruct<vec2>(fallback));
	}

	const rule push_pull_syntax = word >> lit("l") >>
			float_ >> lit("P") >> float_ >> lit("m") >> float_;

	puppet::push_to inline convert(const token& t, const puppet::push_to& fallback) {
		return reparse_convert(push_pull_syntax, t,
			LinearStruct<puppet::push_to>(fallback));
	}
	puppet::tied_to inline convert(const token& t, const puppet::tied_to& fallback) {
		return reparse_convert(push_pull_syntax, t,
			LinearStruct<puppet::tied_to>(fallback));
	}
	
	///ignores value of fallback.auxiliary	
	material inline convert(const token& t, const material& fallback) {
		std::string texture_path = std::string(t.text);
		return {
				.texture = get_texture(texture_path),
				.auxiliary = get_auxilary_texture(texture_path)
		};
	}

	puppet::atlantem inline convert(const token& t, const puppet::atlantem& fallback) {
		if (t.children.size() != 2) std::println("internal error: atlantem parsing broken");
		std::string atlas_name = std::string(t.children.at(0).text);
		decltype(auto) params = t.children.at(1).children;
		std::vector<puppet::formula> formulas;
		formulas.reserve(params.size());
		std::ranges::transform(params, std::back_inserter(formulas), [](const token& t){
			return std::string(t.text);
		});
		return {atlas_name, formulas};
	}

	///ignores fallback's value except if the parse fails
	///XXX Creates a dubious default if the prase succeeds but the conversions fails. 
	puppet::texture_like inline convert(const token& t,
		const puppet::texture_like& fallback) {
		const rule atlantem_syntax =
			word >> lit("[") >> (rex("[^[:space:],\\]]+") % lit(",")) >> lit("]");
		const rule material_syntax = rex("\\./[^[:space:]\\][,]+"); 
		const rule texture_like_syntax = material_syntax || atlantem_syntax;
		std::tuple fallback_partial = {material{},  puppet::atlantem{}}; 
		///uses our fake fallback if parse succeeds and parse fails.
		std::variant data = reparse_convert(texture_like_syntax, t,
			fallback_partial, fallback.data);

		return { data };
	}

	puppet::elbow_ik inline convert(const token& t, const puppet::elbow_ik& fallback) {
		const rule syntax = float_ >> lit(",") >>
				word >> lit(",") >> word;
		return reparse_convert(syntax, t, LinearStruct<puppet::elbow_ik>(fallback));
	}

	puppet::motte inline convert(const token& t, const puppet::motte& fallback) {
		std::vector<puppet::tied_to> ties;
		std::vector<puppet::push_to> pushes;
		//keep track on if they are set, and if so
		//replace fallback with the set value
		//otherwise sum them.
		float friction = fallback.friction;
		vec2 gravity = fallback.gravity;
		for (auto tt : t.children) {
			if (tt.children.size() != 2) {
				std::println(stderr, "WARNING: wrong number"
				" of children for motte parse. {} instead of 2 at {}",
				tt.children.size(), tt.start_pos);
				pretty_print(t);
			}
			token name = tt.children.front();
			token value = tt.children.back();
			if (name.text == "tied_to") {
				ties.emplace_back(convert_delay_define(value, fallback.ties.front()));	
			} else if (name.text == "push_to") {
				pushes.emplace_back(convert_delay_define(value, fallback.pushes.front()));	
			} else if (name.text == "friction") {
				friction = convert_delay_define(value, fallback.friction);	
			} else if (name.text == "gravity") {
				gravity = convert_delay_define(value, fallback.gravity);	
			} else {
				std::println(stderr, "'{}' is an unknown command for mottes at {}; skipping.",
					name.text, name.start_pos);
				
			}
		}
		return {ties, pushes, friction, gravity};
	}

	puppet::joint inline convert(const token& t, const puppet::joint& fallback) {
		std::optional<puppet::elbow_ik> ik = {};
		std::optional<puppet::formula> ft = {};
		if (t.children.size() > 1) {
			std::println(stderr, 
				"Warning: more than one constraint on join, isn't meaningful"	);
		}
		for (auto tt : t.children) {
			if (tt.children.size() != 2) {
				std::println(stderr, "WARNING: wrong number"
				" of children for joint parse. {} instead of 2 at {}",
				tt.children.size(), tt.start_pos);
			}
			token name = tt.children.front();
			token value = tt.children.back();
			if (name.text == "elbow_ik") {
				ik = convert_delay_define(value, *fallback.elbow_ik);	
			} else if (name.text == "fixed_to") {
				ft = convert_delay_define(value, *fallback.fixed_to);	
			}
		}
		return {ik, ft};
	}

	//this also gets executed for all unsigned ints which is perhaps
	//not the best, but it works currently
	//a potential fix is to make all convertion types use material instead
	//but that is overkill for most usages
	GLuint inline convert(const token& t, const GLuint& fallback) {
		return get_texture(std::string(t.text));
	}

	myGl::depthFunc inline convert(const token& t, const myGl::depthFunc& fallback) {
		using namespace std::literals;
		const std::unordered_map<std::string_view, GLenum> gl_map = {
			{"NEVER", GL_NEVER},
			{"LESS", GL_LESS},
			{"EQUAL", GL_EQUAL},
			{"LEQUAL", GL_LEQUAL},
			{"GREATER", GL_GREATER},
			{"NOTEQUAL", GL_NOTEQUAL},
			{"GEQUAL", GL_GEQUAL},
			{"ALWAYS", GL_ALWAYS},
		};

		namespace r = std::ranges;

		std::string uc_text = r::to<std::string>(
			r::views::transform(t.text,
    	[](unsigned char c){ return std::toupper(c); })
		);

		if (gl_map.contains(uc_text)) {
			return { gl_map.at(uc_text) };	
		} else{
			std::println(stderr, "\"{}\" is not a known OpenGL depth Function. The options are {}", t.text, r::views::keys(gl_map)) ;
			
			return fallback;
		}
	}

	///this overload is for parsing the particular field inside the ribbon raw renderable
	std::variant<vec2, puppet::infer> inline convert(const token& t, const std::variant<vec2, puppet::infer>& fallback) {
		if (t.text == "infer") {
			return puppet::infer();
		} else {
			return convert_delay_define(t, std::get<vec2>(fallback) );
		}
	} 

	puppet::atlas inline convert(const token& t, const puppet::atlas& fallback) {
		if (t.children.empty()) return fallback;		
		puppet::atlas at;

		std::optional<size_t> coords_length;

		for (auto& tt : t.children) {
			if (tt.children.size() != 2) {
				std::println(stderr, 
"parsing error: incorrect parse tree for atlas. "
"Must have exactly two children per children. Using fallback atlas.");
				return fallback;
			}
			const token& image_path = tt.children.at(0);
			const token& coords_str = tt.children.at(1);

			const rule vec_point_syntax = float_ % lit(",");
			std::vector<float> point = reparse_convert(vec_point_syntax,
					 coords_str, std::vector<float>{0});

			//handle the error of an inconsistant dimention for coordinates
			//first time
			if (!coords_length) coords_length = point.size();
			else { //rest of points
				if (*coords_length != point.size()) {
					std::println(stderr, "'{}' as a different dimention ({}) than the first coordinate's dimention of {}; skipping", coords_str.text, point.size(), *coords_length);
					continue;
				}
			}

			//Add this cooordinate (and whatever it points to)
			at.push_back(point, convert_delay_define(image_path, GLuint{0}));
			
		}
		return at;	
	}

	puppet::line inline convert(const token& t, const puppet::line& fallback) {
		const rule syntax = int_ > lit(":") > parsing_bits::anything;
		return reparse_convert(syntax, t, LinearStruct<puppet::line>(fallback));
	}

	puppet::history inline convert(const token& t, const puppet::history& fallback) {
		const rule syntax = int_ > lit(":") > parsing_bits::anything;
		return reparse_convert(syntax, t, LinearStruct<puppet::history>(fallback));
	}

	puppet::spring_chain inline convert(const token& t,
			const puppet::spring_chain& fallback) {
		const rule vec2_syntax = float_ > lit(",") > float_;
		const rule syntax = int_ > lit(":") > float_ > lit("{") > vec2_syntax > lit("}") > lit("l") > float_ > lit("P") > float_ > lit("m") > float_;
		return reparse_convert(syntax, t, LinearStruct<puppet::spring_chain>(fallback));
	}


	puppet::spine inline convert(const token& t, const puppet::spine& fallback) {
		puppet::spine our_spine{};
		//keep track on if they are set, and if so
		//replace fallback with the set value
		//otherwise sum them.
		bool set_start = false;
		for (auto tt : t.children) {
			if (tt.children.size() != 2) {
				std::println(stderr, "WARNING: wrong number"
				" of children for motte parse. {} instead of 2 at {}",
				tt.children.size(), tt.start_pos);
			}
			token name = tt.children.front();
			token value = tt.children.back();
			if (name.text == "start") {
				if (set_start) {
					std::println(stderr, "multiple starts given for '{}', using last", t.text);
				}
				our_spine.start = convert_delay_define(value, std::string("not_set"));
			} else if (name.text == "line") {
				our_spine.instructions.emplace_back(convert_delay_define(value, puppet::line{}));	
			} else if (name.text == "history") {
				our_spine.instructions.emplace_back(convert_delay_define(value, puppet::history{}));	
			} else if (name.text == "spring_chain") {
				our_spine.instructions.emplace_back(convert_delay_define(value, puppet::spring_chain{}));	
			} else {
				std::println(stderr, "'{}' is an unknown instruction for spines at {}; skipping.",
					name.text, name.start_pos);
				
			}
		}
		return our_spine;
	}
}
// vim: ts=2 sw=2
