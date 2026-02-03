#include <fstream>
#include <string>
#include <print>
#include <string_view>
#include <unordered_map>
#include "bogeygirl/bogeygirl.hpp"
#include "bogeygirl/gun.hpp"
#include "order_map.h"
#include "renderables.h"
#include "textures.h"
#include "filelayer.h"
#include "puppet.h"
#include "puppet-convert.h"
#include "parsing_bits.h"
BOGEYGIRL_GUN_EXTEND_END //we are done overloading bogeygirl::gun::convert now

using namespace std::literals;

static_assert(bogeygirl::gun::MinimalMap<order_map<int, int>>);

namespace puppet {
	using parsing_bits::newln_or_c;

	auto constexpr section_format(const std::string_view title) {
		using namespace bogeygirl;
		return lit("===") >> lit(title) >> lit("===") >> newln_or_c;
	}

	auto constexpr header_with_type(const std::string_view type) {
		using namespace bogeygirl;
		return lit("[") >> lit(type) >> lit(":") >> balanced_everything_but("#") >> lit("]") >> -word >> newln_or_c;
	}

	auto constexpr parse_format() {
		using namespace bogeygirl;
		using parsing_bits::anything;
		using parsing_bits::pair;

		//XXX maybe rewrite this to prep for a MapStruct format
		//so the sections can appear in any order in the document
		//we shouldn't need to worry about ordering problems
		//because the MapStruct will load the elements in a strict order

		// or perhaps it would be nice to allow multiple of each of type of section
		// and then those sections would be merged at parse time
		
		rule image = header_with_type("img") >> *pair;
		rule images = section_format("images") >> *image;

		rule atlas_index = header_with_type("index") >> *pair;
		rule atlases = section_format("atlases") >> *atlas_index;
		
		rule motte = header_with_type("motte") >> *pair;
		rule joint = header_with_type("joint") >> *pair;
		//eventually to be implemented for this section or (as forces on mottes??):
		rule mottes = section_format("mottes") >> *copy_alt_down(motte || joint);

		rule spine = header_with_type("spine") >> *pair;
		rule skeletons = section_format("skeletons") >> *spine;

		rule breast = header_with_type("breast") >> *pair;
		rule chip = header_with_type("chip") >> *pair;
		rule dot = header_with_type("dot") >> *pair;
		rule ribbon = header_with_type("ribbon") >> *pair;
		rule chip_strip = header_with_type("chip_strip") >> *pair;
		rule render_loop = section_format("render") >> *(chip||breast||dot||ribbon||chip_strip);

		rule document = -newln_or_c >>
			images >> atlases >> mottes >> skeletons >> render_loop;

		return document;
	}


	template<class T> using parents_t = std::unordered_map<std::string_view, T>;

	///create the fallback structure which offers inheritance for our format
	template<class T>
	auto inheritance_fallback(const T& fallback, parents_t<T>& parents) {
			using namespace bogeygirl::gun;
			return FillMap{
				ReplaceFallback{
					NthChild{2,
						MapStruct{fallback}
					},
				&parents, {1}	
				},
				&parents, {0}
			};
	}

	std::string parse_atlas_header(const bogeygirl::token& t) {
		static int default_counter;
		using namespace bogeygirl;
		rule the_rule = word > lit("[") > flatten(word >> *(lit(",") >> word)) >> lit("]");	
		std::optional result = prase(the_rule, space_skip, t.text);
		if (result) return std::string(result->children.at(0).text);
		else {
			std::println("invalid atlas name: {}", t.text);
			return std::format("default-atlas-name-{}", default_counter++);
		}
	}

	puppet prase_fields(bogeygirl::token parse_tree) {
		using namespace bogeygirl::gun;
		puppet result;

		//the renderables	
		chip the_chip = {
			material{
			get_texture("./ui/face-arrow.png"),
			get_texture("./ui/default-auxiliary.png"),
			},
			"head",
			"neck",
			"0.5",
			0.01f,
			0.6f,
			GL_GEQUAL,
			0.0f,
			0.0f,
			{1.00, 1.00, 1.00},
		};
		parents_t<chip> chips_parents;
		auto chip_fallback = inheritance_fallback(the_chip, chips_parents);
		
		dot the_dot = {
			material{
			get_texture("./ui/face-arrow.png"),
			get_texture("./ui/default-auxiliary.png"),
			},
			"head",
			"0.5",
			0.01f,
			0.6f,
			GL_GEQUAL,
			{1.00, 1.00, 1.00},
		};
		parents_t<dot> dots_parents;

		auto dot_fallback = inheritance_fallback(the_dot, dots_parents);

		breast the_breast = {
				material{
				get_texture("./ui/face-arrow.png"),
				get_texture("./ui/default-auxiliary.png"),
				},
				"head",
				"neck",
				"0.5",
				0.01f,
				0.6f,
				GL_GEQUAL,
				0.0f,
				0.0f,
				{1.00, 1.00, 1.00},
				"thorax"
		};
		parents_t<breast> breasts_parents;
		auto breast_fallback = inheritance_fallback(the_breast, breasts_parents);

		ribbon the_ribbon = {
			material{
			get_texture("./ui/face-arrow.png"),
			get_texture("./ui/default-auxiliary.png"),
			},
			"[unset_spine]",
			"0.5",
			vec2{1.0, 1.0},
			0.6f,
			0.9f,
			GL_GEQUAL,
			{1.00, 1.00, 1.00},
		};
		parents_t<ribbon> ribbons_parents;
		auto ribbons_fallback = inheritance_fallback(the_ribbon, ribbons_parents);

		chip_strip the_chip_strip = {
			material{
			get_texture("./ui/face-arrow.png"),
			get_texture("./ui/default-auxiliary.png"),
			},
			"[unset_spine]",
			"0.5",
			0.6f,
			0.9f,
			GL_GEQUAL,
			0.0f,
			0.0f,
			{1.00, 1.00, 1.00}
		};
		parents_t<chip_strip> chip_strip_parents;
		auto chip_strip_fallback = inheritance_fallback(the_chip_strip, chip_strip_parents);

		//----
		auto toRender_item =  std::tuple { chip_fallback, breast_fallback,
				dot_fallback, ribbons_fallback, chip_strip_fallback };  
		using toRender_item_t = decltype(toRender_item);

		auto toRender_fallback =
			order_map<std::string, MapPathGuide<toRender_item_t>>
				{{"key", {toRender_item, {}}}};
				
		//skeletons (i.e. spines for now)
		spine the_spine = {"head", {}};
		parents_t<spine> spine_parents;
		auto spine_item = inheritance_fallback(the_spine, spine_parents);
		auto skeletons_fallback = order_map<std::string, MapPathGuide<spine> >
			{{"unset_name", {the_spine, {2}}}};
	
		// --- the mottes section	 ---
		//mottes
		tied_to fallback_tied = {"head", 0.1, 0.7, 0.2};
		push_to fallback_push = {"head", 0.1, 0.7, 0.2};
		motte the_motte = {{fallback_tied}, {fallback_push}, 0.3, {0, -0.01}};

		//joint
		joint the_joint{{{}}, {"{0,0}"}};

		auto mj_item =  std::tuple { the_motte, the_joint};  
		using mj_item_t = decltype(mj_item);
		auto mj_fallback = order_map<std::string, MapPathGuide<mj_item_t>>
				{{"key", {mj_item, {2}}}};

		//atlases	
		atlas the_atlas{{{0.0f},0}};
		parents_t<atlas> atlases_parents;
		auto atlases_item = inheritance_fallback(the_atlas, atlases_parents);
		auto atlases_fallback = order_map<UseFunc<std::string>, MapPathGuide<atlas> >
			{{{"unset_name", parse_atlas_header}, {the_atlas, {2}}}};
		//TODO we need to actually reparse parse the name 

		//keypoints
		using keypoint_t = std::unordered_map<std::string, formula>;
		keypoint_t keypoint_item = {{"unset_name", "{0,0}"}};
		order_map<GLuint, MapPathGuide<keypoint_t>> 
			keypoints_fallback = {{0, {keypoint_item, {2}}}}; 
		
				
		result.keypoints = convert(*parse_tree.get({0,0}), keypoints_fallback);
		result.atlases = convert(*parse_tree.get({1,0}), atlases_fallback);
		result.mottes = convert(*parse_tree.get({2,0}), mj_fallback);
		result.skeletons = convert(*parse_tree.get({3,0}), skeletons_fallback);
		result.toRender = convert(*parse_tree.get({4,0}), toRender_fallback);

		return result; 
	}

	std::optional<puppet> load(std::filesystem::path filename) {
		const std::filesystem::path path = filelayer::canonise_path(filename);
		std::ifstream file(path);
		if (file.fail()) {
			std::println(stderr, "could not open puppet file: {}", filename.string());
			return {};
		}

		std::string content{std::istreambuf_iterator<char>(file),
				 std::istreambuf_iterator<char>()};

		std::optional<bogeygirl::token> maybe_parse_tree
			= bogeygirl::prase(parse_format(), bogeygirl::space_skip, content);

		if (!maybe_parse_tree) return {}; //no need to print error message
		//bogeygirl::parse already did.
	
		bogeygirl::token parse_tree = *maybe_parse_tree;
		#ifdef SELF_TEST
		std::println("# parse tree");
		bogeygirl::pretty_print(parse_tree);
		#endif
		
		return prase_fields(parse_tree);	
	};
}

#ifdef SELF_TEST
int main() {
	std::println("start puppet");
	std::optional maybe_puppet = puppet::load("kantan-chan/puppet.pup");

	if (!maybe_puppet) {
		std::println("parsing failed");
		return 1;
	}

	puppet::puppet a = *maybe_puppet;

	std::println("-- images --");	
	for (auto [key,i] : a.keypoints) {
		std::println("for img \"{}\":", key);
		for (auto [name, form] : i) 	{
			std::println("\t{} = {}", name, form);
		}	
	}

	std::println("-- atlases --");
	for (auto [name, atlas] : a.atlases) {
		std::println("for  \"{}\":", name);
		for (auto [point, img] : atlas) 	{
			std::println("\t{} = {}", point, img);
		}	
	}
	
	std::println("-- skeleton --");
	for (auto [name, skeleton] : a.skeletons) {
		std::println("for  \"{}\":", name);
		std::println("\tstart:", skeleton.start);
		
		for (const auto& instruction : skeleton.instructions) 	{
			std::visit([](auto&& a){std::println("\tlength:{}", a.length);}, instruction);
		}	
	}

	std::println("--motes--");
	for (auto [key,mj] : a.mottes) {
		if (std::holds_alternative<puppet::motte>(mj)) {
			const puppet::motte& i = get<puppet::motte>(mj);
			std::println("motte");
			std::println("name: {}", key);
			std::println("gravity: {}", i.gravity);
			std::println("friction: {}", i.friction);
			for (auto j : i.ties) {
				std::println("tied_to: {}, {}, {}, {} EOL",
					j.target, j.pull_length, j.power, j.max_length);
			}
			for (auto j : i.pushes) {
				std::println("push_to: {}, {}, {}, {} EOL",
					j.target, j.push_length, j.power, j.min_length);
			}
			std::println("--");
		} else if (std::holds_alternative<puppet::joint>(mj)) {
			const puppet::joint& i = get<puppet::joint>(mj);
			std::println("joint");
			std::println("name: {}", key);
		}
	}
	std::println("--renderables--");
	for (auto [key, render] : a.toRender) {
		if (std::holds_alternative<puppet::ribbon>(render)) {
			const puppet::ribbon r = std::get<puppet::ribbon>(render);
			std::println("ribbon: {}", key);
			std::println("width: {}", r.width);
			std::println("spine: {}", r.spine);
			std::println("flip: {}", r.flip);
		}
	}
	std::println("end of puppet");
}
#endif
// vim: ts=2 sw=2
