#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "ndim_knn_map.h"
#include "renderables.h"
#include "vec.h"
#include "phys.h"
#include "order_map.h"
#include <variant>
#include <optional>
#include "myGl.h"
#include <format>
#include <vector>
#include <filesystem>

namespace puppet {
	//this is a math formula with references to variables
	//TODO at some point this should be changed not compile the formula everytime
	using formula = std::string;
	using formula1d = std::string;
	using spine_label = std::string;

	///all the data needed turn the symbolic formulas into actual values
	struct live_data {
		std::unordered_map<std::string, struct phys> phys;
		std::unordered_map<std::string, float> floats;
		std::unordered_map<std::string, std::vector<struct phys>> spines;
	};
	

	///maps a N-coordinate point to a texture
	using atlas =	ndim_knn_map<GLuint>;

	///'atlantem' is the accusative case of Atlas
	/// and it represents the call to an atlas to get a material
	struct atlantem {
		std::string atlas_name;
		std::vector<formula> coords;
	};

	//using atlas_store = std::unordered_map<std::string, atlas>;
	//template<class T> concept atlas_store = Maplike<std::string, atlas, T>;
	using atlas_store = order_map<std::string, atlas>;
	using texture_keypoints = std::unordered_map<std::string, formula>;  
	using keypoints_store = order_map<GLuint, std::unordered_map<std::string, formula> >;

	///it goes in the texture slot
	///even if it can represent a 
	//we use a struct to get a distinct type.
	struct texture_like {
		std::variant<material, atlantem> data;
		material eval(
			const atlas_store& atlases,
			const live_data &live) const;
	};

	struct dot {
//		std::unsorted_set<std::string> requirement;
		texture_like texture; //called "texture" because that is the primary usage
		formula center;
		formula1d radius;
		float back;
		float front;
		myGl::depthFunc depthFunc;
		vec3 whiteColor;
	};

	/// a image defined by its end points and width
	/// can be processed to a chip ready to render
	struct chip {
//		std::unsorted_set<std::string> requirement;
		texture_like texture; //called "texture" because that is the primary usage
		formula top;
		formula bottom;
		formula1d width;
		float back;
		float front;
		myGl::depthFunc depthFunc;
		float topOverdraw;
		float bottomOverdraw;
		vec3 whiteColor;
	};

	/// a image defined by its end points and width, with a center
	struct breast {
		texture_like texture;
//		std::unsorted_set<std::string> requirement;
		formula top;
		formula bottom;
		formula1d width;
		float back;
		float front;
		myGl::depthFunc depthFunc;
		float topOverdraw;
		float bottomOverdraw;
		vec3 whiteColor;
		formula middle;
	};

	///tag type which means to infer the (cross_direction) from local context
	struct infer{};
	
	///an image stretched over a spine
	struct ribbon {
		//std::unsorted_set<std::string> requirement;
		texture_like texture; //called "texture" because that is the primary usage
		spine_label spine;	
		formula1d width;
		std::variant<vec2, infer> cross_direction; // if there is no cross direction,
		                                           // then it is the local one
		float back;
		float front;
		myGl::depthFunc depthFunc;
		vec3 whiteColor;
		bool flip; //if we should flip the image lengthwise
	};

	///a series of chips arranged end to end drawn on a spine
	struct chip_strip {
		texture_like texture; //called "texture" because that is the primary usage
		spine_label spine;	
		formula1d width;
		float back;
		float front;
		myGl::depthFunc depthFunc;
		float topOverdraw;
		float bottomOverdraw;
		vec3 whiteColor;
	};

	///motte to motte relationship; pull together
	struct tied_to {
		formula target;
		float pull_length;
	 	float power;	
		float max_length;
	};
	///motte to motte relationship; push together
	struct push_to {
		formula target;
		float push_length;
	 	float power;	
		float min_length;
	};

	//ik specialized to be reasonable for elbows
	struct elbow_ik {
		float length;
		formula shoulder;
		formula hand;
	};

	///a physics point which things are rigged to
	struct motte {
		std::vector<tied_to> ties;
		std::vector<push_to> pushes;
		float friction;
		vec2 gravity;
	};

	//a rigging point that that has no momentum
	struct joint {
		std::optional<struct elbow_ik> elbow_ik;
		std::optional<formula> fixed_to;
	};

	///a spine segment which reflects the history of its start point
	struct history { 
		size_t length;
		formula offset;	//offest * index_of_point
	};

	///a spine segment which is a linear link between a start and end point
	struct line {
		size_t length;
		formula end;
	};

	///a spine segment which is a stretchy chain of mottes
	///it does not conserve energy or do any of the stuff, 
	///one might expect from a phys sim. But that is okay, because we are 
	///doing of animation, and realistic physics don't look that good anyway.
	struct spring_chain {
		size_t length;
		float friction;
		vec2 gravity;
		float pull_length;
	 	float pull_power;	
		float max_length;
	};


	///a chain of motes
	struct spine { 
		formula start;

		//each member must have a size_t length field
		std::vector<std::variant<history, line, spring_chain>> instructions;
		
		size_t size() const;
	};


	using typesOfRawRenderable = std::variant<chip, breast, dot, ribbon, chip_strip> ;
	using typesOfMotte = std::variant<motte, joint> ;

	struct puppet {
		//this of everything that must be rendered
		//and is in order that it is rendered
		order_map<std::string, typesOfRawRenderable> toRender;
		//maps file name to point relative to top left
		keypoints_store keypoints;
		
		order_map<std::string, typesOfMotte> mottes;
		order_map<std::string, atlas> atlases;
		order_map<std::string, spine> skeletons;
	};
	std::optional<puppet> load(std::filesystem::path filename);
	
	renderables render(puppet m, live_data &phys_values);
}


template <>
struct std::formatter<puppet::live_data> {
	bool long_print = false;

	constexpr auto parse(std::format_parse_context& ctx) {
			auto it = ctx.begin();	
			if (it == ctx.end()) return it;
			if (*it == '?') {
				long_print = true;
				++it;
			}
			return it;
	}

	auto format(const puppet::live_data& data, std::format_context& ctx) const {
			auto result = ctx.out();
			result = std::format_to(result, "live-data:[#phys: {}, #floats: {}, #spines: {}]",
				data.phys.size(), data.floats.size(), data.spines.size()
			);
			
			result = std::format_to(result, "\n");
			result = std::format_to(result, "\tphys:\n");
			for (const auto& [key, phy] : data.phys) {
				result = std::format_to(result, "\t  {}: {}\n", key, phy);
			}

			result = std::format_to(result, "\tfloats:\n");
			for (const auto& [key, f] : data.floats) {
				result = std::format_to(result, "\t  {}: {}\n", key, f);
			}
			
			result = std::format_to(result, "\tspines:\n");
			for (const auto& [key, s] : data.spines) {
					result = std::format_to(result, "\t{}:", key);
					for (const auto& [k, ss] : s) {
						result = std::format_to(result, "\t\t{}", ss);
					}
			}
			
			return result;

	}
};

// vim: ts=2 sw=2
