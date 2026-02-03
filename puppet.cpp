#include "puppet.h"
#include <algorithm>
#include <exception>
#include <print>
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>

#include "libs/muparserx/parser/mpParser.h"
#include <ranges>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

#include "libs/muparserx/parser/mpTypes.h"
#include "libs/muparserx/parser/mpValue.h"
#include "math_util.h"
#include "renderables.h"
#include "textures.h"
#include "vec.h"
#include "phys.h"

#include "data_prefixes.h"

using boost::regex;
using boost::cmatch;

namespace puppet {
	//see 'puppet-load.cpp' for puppet::load

	static float evalCalc1d(const formula& formula, const live_data& data,
			 std::string_view hint = "[unknown context]") {
		//looks like I am parsing the formula every single frame
		//this is suboptimal since the calc library supports parsing and then storing them
		
		try {
			using namespace mup;
			ParserX p;
			p.SetExpr(formula);

			//define all the variables
			var_maptype vmap = p.GetExprVar();
			std::vector<Value> values;
			values.reserve(vmap.size());
			for (var_maptype::iterator item = vmap.begin(); item!=vmap.end(); ++item) {
				std::string name = item->first;

				Value fVal;
				if (data.floats.contains(name)) {
					fVal = Value(data.floats.at(name));
				} else if (name.starts_with(time_prefix)) {
					///we create time data from the void here, on use.
					///This does mean that time_variables don't live inside live_data
					///which is perhaps confusing

					//find the name after the time prefix
					auto time_paramater = name.substr(strlen(time_prefix), name.size() - 1);
					//TODO this will throw and thus crash on invalid number
					int ms_in_loop = std::stoi(time_paramater);
			
					//find current time in ms	
					auto now =
						std::chrono::high_resolution_clock().now();
					auto abs_ms =
						std::chrono::time_point_cast<std::chrono::milliseconds>(now)
							.time_since_epoch().count();
					/// now loop the current time from 0 to 1 over ms_in_loop milliseconds
					float t_from_0_to_1 = float(abs_ms % ms_in_loop) / ms_in_loop;
					fVal = Value(t_from_0_to_1);
					
				} else if (name.starts_with(keyboard_prefix)) {
					///for now we silently ignore missing keyboard values
					///even if such a key can never be pressed. This is suboptimal.
					fVal = Value(0.0f);
				} else {
					std::println("in hint {}, '{}' name not found in supplied variables",
						hint, name);
					fVal = Value(0.0f);
				}
				values.push_back(fVal);
				p.DefineVar(name, Variable(&values.back()));
			}
			
			Value result = p.Eval();
			float fVal = result.GetFloat();
			return fVal;

		} catch (mup::ParserError e) {
			std::println(stderr,
				"1d math error: {} ~ {} ~ {} hint: {}", e.GetMsg(), e.GetToken(), e.GetPos(), hint );
			return 0.0;
		}
	}

	static vec2 evalCalc2d(const formula& formula, const live_data& data, std::string_view error_context = "[unknown context]") {
		//looks like I am parsing the formula every single frame
		//this is suboptimal since the calc library supports parsing and then storing them
		
		try {
			using namespace mup;
			ParserX p;
			p.SetExpr(formula);

			//define all the variables
			var_maptype vmap = p.GetExprVar();
			std::vector<Value> values;
			values.reserve(vmap.size());
			for (var_maptype::iterator item = vmap.begin(); item!=vmap.end(); ++item) {
				std::string name = item->first;

				if (data.phys.contains(name)) {
					vec2 pos = data.phys.at(name).p;
					Value m1 = Value(1, 2, 0);
					m1.At(0, 0) = pos[0];
					m1.At(0, 1) = pos[1];
					values.push_back(m1);
					p.DefineVar(name, Variable(&values.back()));
				} else if (data.floats.contains(name)) {
					float f = data.floats.at(name);
					values.emplace_back(f);
					p.DefineVar(name, Variable(&values.back()));

				} else {
					std::println("'{}' name not found in supplied variables", name);
				}
			}
			
			Value result = p.Eval();
			vec2 pos = {(float) result.At(0, 0).GetFloat(), (float) result.At(0, 1).GetFloat()};
			return pos;

		} catch (mup::ParserError e) {
			std::println(stderr, "2d math error: {} ~ {} ~ {}, clue: {}", e.GetMsg(), e.GetToken(), e.GetPos(), error_context);
			return {0.5,0.5};
		}
	}

	// ()() <- fanservice to help hold your attention
	// no nipples because we're merely etchi not hentai

	material texture_like::eval(
			const atlas_store& atlases,
			const live_data &live) const {
			if (std::holds_alternative<material>(data)) return std::get<material>(data);
			//must hold atlas	now
			const atlantem& atlm = std::get<atlantem>(data);
			if (!atlases.contains(atlm.atlas_name)) {
				std::println("invalid atlas reference, unknown atlas name '{}'",
						 atlm.atlas_name);
					return {};
			};
			const atlas& a = atlases.at(atlm.atlas_name);

			if (a.dimentions() != atlm.coords.size()) {
				std::println("invalid atlas reference, different lengths: "
				             "expected {} != supplied {}",
					a.dimentions(), atlm.coords.size());
					return {};
			}
		
			auto coords = std::ranges::transform_view(atlm.coords,
				[&](std::string x) -> float { return evalCalc1d(x, live); }
			);
			std::vector<float> coords_vec = std::ranges::to<std::vector>(coords);
			GLuint primary_texture = a.get(coords_vec);
			GLuint auxillary_texture = get_auxilary_texture(primary_texture);
			return material{primary_texture, auxillary_texture};
	}


	///executed every frame; 
	///ready the elements to render
	///and update any internal model elements (read-written to data)
	renderables render(puppet pup, live_data &data) {
		using std::get;

		////physics
		//execute mottes
		for (const auto& [name, particle] : pup.mottes) {
			if (std::holds_alternative<motte>(particle)) {
				const motte& m = get<motte>(particle);
				phys& node = data.phys[name];
				node.v += m.gravity;	
				node.v -= m.friction*expm1(node.v);
				
				for (const auto& t : m.ties) {
					node.tied_to(t.pull_length, evalCalc2d(t.target, data, "tied_to"), t.power, t.max_length);
				}  
				
				for (const auto& p : m.pushes) {
					node.push_to(p.push_length, evalCalc2d(p.target, data, "push_to"), p.power, p.min_length);
				}

				node.haps_reset();
				node.play();
			}
			else if (std::holds_alternative<joint>(particle)) {
				const joint& j = std::get<joint>(particle);
				phys& node = data.phys[name];

				//TODO maybe change to variant instead of set of optionals?
				if (j.elbow_ik) {
					auto e = *j.elbow_ik;
					node.elbow_ik(e.length, evalCalc2d(e.shoulder, data, "elbow_ik shoulder"),
																	 evalCalc2d(e.hand, data, "elbow_ik hand"));
				}

				if (j.fixed_to) {
					std::string f = *j.fixed_to;
					node.wind(evalCalc2d(f, data, name));
				}
				node.haps_reset();
				node.v = {0,0};
			}
		}

		//execute spines
		for (const auto& [name, spine] : pup.skeletons) {
			auto& nodes = data.spines[name];
			//initialize nodes if it isn't already
			while (nodes.size() < spine.size()) {
				nodes.push_back({});
			}
			vec2 start = evalCalc2d(spine.start, data, name);
			nodes[0].wind(start);
			size_t data_index = 1;
			for (auto&& instruction : spine.instructions) {
				if (std::holds_alternative<line>(instruction)) {
					auto&& l = get<line>(instruction);
					vec2 end = evalCalc2d(l.end, data, name);
					for (size_t i = data_index; i <= l.length + data_index; ++i) {
						nodes[i].wind(start.towards(
							static_cast<double>(i)/l.length, end)
						);
					}
				} else if (std::holds_alternative<history>(instruction)) {
					auto&& hist = get<history>(instruction);
					vec2 offset = evalCalc2d(hist.offset, data, name);
					//walk backwards copying forwards
					for (size_t i = data_index + hist.length - 1; i > data_index; --i) {
						nodes[i].wind(nodes[i-1].p + offset);	
					}
				} else if (std::holds_alternative<spring_chain>(instruction)) {
					auto&& sc = get<spring_chain>(instruction);
					for (size_t i = data_index; i < data_index + sc.length; ++i) {
						phys& node = nodes[i];
						phys& last_node = nodes[i-1];
						node.v += sc.gravity;	
						node.v -= sc.friction*expm1(node.v);
						
						node.tied_to(sc.pull_length, last_node.p, sc.pull_power, sc.max_length);
						
						node.haps_reset();
						node.play();
					}
				}
				data_index += std::visit([](auto a){return a.length;}, instruction);
			}
		}

		//render
		std::vector<noodle> noodles;
		for (const auto& [name, rawRenderable] : pup.toRender) {
			if (std::holds_alternative<chip>(rawRenderable)) {
				const chip& ch = std::get<chip>(rawRenderable);

				vec2 top = evalCalc2d(ch.top, data, "chip top");
				vec2 bottom = evalCalc2d(ch.bottom, data, "chip bottom");
				float width = evalCalc1d(ch.width, data, "chip width");

				top = top.towords(-ch.topOverdraw, bottom);
				bottom = bottom.towords(-ch.bottomOverdraw, top);
				material mat = ch.texture.eval(pup.atlases, data);
				
				noodles.push_back({
						mat,	
						{bottom, top},
						width,
						ch.back,
						ch.front,
						ch.depthFunc,
						ch.whiteColor,
						false,
						(top - bottom).perpen().norm()
						});
				
				//run update the image keypoints
				for (auto [key,string_pos] : pup.keypoints[mat.texture]) {
					vec2 pos = evalCalc2d(string_pos, data); 
					vec2 parallel = bottom - top;
					vec2 perpendicular = -parallel.perpen().norm() * width;
					vec2 newpoint = pos.emplace(perpendicular, parallel, 
							top - perpendicular	/ 2);
					data.phys[name + "__" + key].p = newpoint;
					data.phys[key].p = newpoint;
				}

			} else if (std::holds_alternative<dot>(rawRenderable)) {
				const dot& d = std::get<dot>(rawRenderable);
				vec2 pos = evalCalc2d(d.center, data);
				material mat = d.texture.eval(pup.atlases, data);
				float radius = evalCalc1d(d.radius, data, "dot radius");

				noodles.push_back({
						mat,
						{pos - vec2{0, radius}, pos + vec2{0, radius}},
						radius*2,
						d.back,
						d.front,
						d.depthFunc,
						d.whiteColor,
						false,
						{1,0}
						});

				//run update the image keypoints
				for (auto [key,string_pos] : pup.keypoints[mat.texture]) {
					vec2 pos = evalCalc2d(string_pos, data); 
					vec2 parallel = vec2{0, radius*2};
					vec2 perpendicular = {radius*2, 0};
					vec2 newpoint = pos.emplace(perpendicular, parallel, 
							pos + vec2{radius, radius});
					data.phys[name + "__" + key].p = newpoint;
					data.phys[key].p = newpoint;
				}
				
			} else if (std::holds_alternative<breast>(rawRenderable)) {
				//breasts 
				//()() with your scroll wheel you can see them jiggle (>ᴗ•)
				const breast& b = std::get<breast>(rawRenderable);

				vec2 top = data.phys[b.top].p;
				vec2 middle = data.phys[b.middle].p;
				vec2 bottom = data.phys[b.bottom].p;

				top = top.towords(-b.topOverdraw, bottom);
				bottom = bottom.towords(-b.bottomOverdraw, top);
				float width = evalCalc1d(b.width, data, "breast width");

				noodles.push_back({
						b.texture.eval(pup.atlases, data),
						{bottom, middle, top},
						width,
						b.back,
						b.front,
						b.depthFunc,
						b.whiteColor,
						false,
						(top - bottom).perpen().norm()
				});

				//Figuring how put image keypoints on breasts correctly is hard
				//so it isn't implemented yet 
			} else if (std::holds_alternative<ribbon>(rawRenderable)) {
				//ribbons: dressing for spines 
				const ribbon& r = std::get<ribbon>(rawRenderable);

				std::vector<phys>& spine_locations = data.spines.at(r.spine);
				float width = evalCalc1d(r.width, data, "ribbon width");

				vec2 cross_direction;
				bool has_direction = false;
				if (std::holds_alternative<vec2>(r.cross_direction)) {
					cross_direction = std::get<vec2>(r.cross_direction);
					has_direction = true;
				} 
				std::vector<vec2> spine_locations_p(spine_locations.size());
				if (r.flip) {
					std::ranges::transform(spine_locations, spine_locations_p.rbegin(), [](phys& x){return x.p;}); 
				} else {
					std::ranges::transform(spine_locations, spine_locations_p.begin(), [](phys& x){return x.p;}); 
				}
			
				//std::ranges::for_each(spine_locations_p, [](auto a){std::print("{},", a);});

				noodles.push_back(noodle{
						r.texture.eval(pup.atlases, data),
						spine_locations_p,	
						width,
						r.back,
						r.front,
						r.depthFunc,
						r.whiteColor,
						!has_direction,
						cross_direction	
					});

				//Figuring how put image keypoints on breasts correctly is hard
				//so it isn't implemented yet 

			} else if (std::holds_alternative<chip_strip>(rawRenderable)) {
				const chip_strip& cs = std::get<chip_strip>(rawRenderable);
				std::vector<phys>& spine_locations = data.spines.at(cs.spine);
				material mat = cs.texture.eval(pup.atlases, data);
				//for all the valid consecuative pairs
				for (size_t i = 0; i < spine_locations.size()-1; ++i) {
					vec2 top = spine_locations[i].p;
					vec2 bottom = spine_locations[i+1].p;
					top = top.towords(-cs.topOverdraw, bottom);
					bottom = bottom.towords(-cs.bottomOverdraw, top);

					float width = evalCalc1d(cs.width, data, "chip_strip width");
					
					noodles.push_back({
							mat,	
							{bottom, top},
							width,
							cs.back,
							cs.front,
							cs.depthFunc,
							cs.whiteColor,
							false,
							(top - bottom).perpen().norm()
					});
				}

				//updating image keypoints is hard / confusing here so we don't do it for now
			} else {
				throw std::logic_error("not an implemented type of renderable:" + name);
			}
		}
		return {noodles};
	};

	size_t spine::size() const {
		size_t size = 1; //we start at 1 to account for the start node
		for (auto&& i : instructions) {
			std::visit([&size](auto&& a){size += a.length;}, i);
		}
		return size;
	}
}
// vim: ts=2 sw=2
