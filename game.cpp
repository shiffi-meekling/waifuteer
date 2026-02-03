#include "game.h"
#include <exception>
#include <string_view>
#include <unordered_map>
#include <SDL.h>
#include <algorithm>
#include "face_tracking.h"
#include <cmath>

#include "render.h"

#include "puppet.h"
#include "stabilizer.h"
#include <print>
#include "edit-notify.h" 
#include "bogeygirl/strlib.hpp"

#include "data_prefixes.h"
#include "filelayer.h"

using std::unordered_map;
using std::max;

static std::optional<puppet::puppet> myPuppet;
static render_world state; 
static puppet::live_data live_data;
static std::filesystem::path puppet_path = "";

void init_game() {
	if (puppet_path.empty()) {
		throw std::logic_error("you must call init_game with arguements at least once first");
	}
	init_game(puppet_path);
}

void init_game(std::filesystem::path puppet_path_arg) {
	puppet_path = filelayer::canonise_path(puppet_path_arg);
	//we want to watch again, because sometimes our file
	//was deleted and replaced with an identically named one.
	edit_notify::start_watch(puppet_path_arg, init_game);
	state.camera_pan = {0,0};
	state.camera_scale = {screen_height/(float) screen_width, 1};
	state.tracking_data = {};
	state.debug = false;
	state.background = 4;
	state.sparkle = 1;

	std::println("parsing puppet");	
	myPuppet = puppet::load(puppet_path);

	live_data = {};
}

void friction(phys& mover, float power) {
	mover.v -= power*expm1(mover.v);
}

vec2 invert_camera(vec2 point) {
	return (point - state.camera_pan.p) / state.camera_scale.p;
}

vec2 simple_ik(float length, vec2 start, vec2 end, float ratio=0.5, bool direction=true) {
	length = length*ratio; //total length -> first unit length;

	vec2 v = end - start;
	vec2 u = v.perpen().norm();
	
	vec2 hv = v*ratio;
	float mag_hv = hv.mag();

	float qq = length*length - mag_hv*mag_hv;
	//try as hard as you can to reach and if you fail thats okay
	qq = max(qq, 0.0f);

	float sign = direction ? 1 : -1;

	return hv + start + sign*u*sqrt(qq);
}


render_world update_gamestate(mouse_input& mouse,
	   	unordered_map<SDL_Keycode, float> &keyboard,
			unordered_map<Uint8, float> &joystick_buttons,
		tracking::metrics tracking_data,
			float mic
    ) {
	state.time++;

	state.tracking_data = tracking_data;
	
	//camera
	if (keyboard[SDLK_KP_PLUS] || keyboard[SDLK_EQUALS]) state.camera_scale.p *= 1.1;
	if (keyboard[SDLK_KP_MINUS] || keyboard[SDLK_MINUS]) state.camera_scale.p *= 0.9;
	float pan_speed = 0.05;
	if (keyboard[SDLK_LSHIFT]) pan_speed = 0.005;
	if (keyboard[SDLK_UP] 
			|| keyboard[SDLK_KP_8]) state.camera_pan.p.y += pan_speed;
	if (keyboard[SDLK_DOWN]
		 	|| keyboard[SDLK_KP_2]) state.camera_pan.p.y -= pan_speed;
	if (keyboard[SDLK_RIGHT]
		 	|| keyboard[SDLK_KP_6]) state.camera_pan.p.x += pan_speed;
	if (keyboard[SDLK_LEFT] 
			|| keyboard[SDLK_KP_4]) state.camera_pan.p.x -= pan_speed;

	//process tracking info
	live_data.phys["_nose"].wind(tracking_data.nose);
	live_data.floats["_mic"] = mic;
	live_data.floats["_mouth_open"] = tracking_data.mouth_open;
	live_data.floats["_mouth_wide"] = tracking_data.mouth_wide;
	live_data.phys["_head_up_unit_vector"].wind(tracking_data.head_up_unit_vector);

	//insert keyboard data
	for (auto [key, value] : keyboard) {
		std::string key_name = SDL_GetKeyName(key);
		strlib::lower(key_name);
		strlib::replace(key_name, ' ', '_');
		std::string formatted_name = keyboard_prefix + key_name;
		live_data.floats[formatted_name] = value;
	}

	//if we successfully loaded the puppet, render it 
	if (myPuppet) state.render = puppet::render(*myPuppet, live_data);

	return state;
}

void keyboard_events(SDL_Event event) {
	//TODO F12 is bound to something else already
	//Gah, maybe I should merge openning the debug window from main into this function :/
	if (event.key.keysym.sym == SDLK_F12) {
		state.debug = !state.debug;
	} else if (event.key.keysym.sym == SDLK_F2) {
		init_game();
	} else if (event.key.keysym.sym == SDLK_F3) {
		state.background = (state.background + 1) % 5;
	} else if (event.key.keysym.sym == SDLK_F4) {
		state.sparkle = (state.sparkle + 1) % 4;
	} 
}

#include "imgui.h"
namespace gui::game {

void live_phys() {
		using namespace ImGui;
		ImGui::Begin("live phys info");
		if (!ImGui::BeginTable("Live phys", 3, ImGuiTableFlags_SizingStretchProp)) { return;}
		TableNextRow();   TableSetColumnIndex(0); Text("name");
											TableSetColumnIndex(1); Text("position");
											TableSetColumnIndex(2); Text("velocity");

		for (auto [name, phys] : live_data.phys) {
			TableNextRow(); TableSetColumnIndex(0); Text("%s", name.c_str());
											TableSetColumnIndex(1); Text("%f,%f", phys.p.x, phys.p.y);  
											TableSetColumnIndex(2); Text("%f,%f", phys.v.x, phys.p.y);  
		}
		ImGui::EndTable();
		ImGui::End();

}

void live_float() {
		using namespace ImGui;
		ImGui::Begin("live float info");
		if (!ImGui::BeginTable("Live float", 2, ImGuiTableFlags_SizingStretchProp)) { return;}
		TableNextRow();   TableSetColumnIndex(0); Text("name");
											TableSetColumnIndex(1); Text("value");

		for (auto [name, f] : live_data.floats) {
			TableNextRow(); TableSetColumnIndex(0); Text("%s", name.c_str());
											TableSetColumnIndex(1); Text("%f", f);  
		}
		ImGui::EndTable();
		ImGui::End();
}

void parsed_puppet() {
	using namespace ImGui;
	if (myPuppet) {
		ImGui::Begin("parsed puppet");
		const puppet::puppet& p = *myPuppet;
			

		Text("-- images --");	
		for (auto [key,i] : p.keypoints) {
			Text("for img \"%d\":", key);
			for (auto [name, form] : i) 	{
				Text("\t%s = %s", name.c_str(), form.c_str());
			}	
		}

		Text("-- atlases --");
		for (auto [name, atlas] : p.atlases) {
			Text("for atlas \"%s\":", name.c_str());
			for (auto [point, img] : atlas) 	{
				Text("\t%s = %d", std::format("{}",point).c_str(), img);
			}	
		}
		
		Text("-- skeleton --");
		for (auto [name, skeleton] : p.skeletons) {
			Text("for  \"%s\":", name.c_str());
			Text("\tstart: %s", skeleton.start.c_str());
			
			for (const auto& instruction : skeleton.instructions) 	{
				std::visit([](auto&& a){Text("\tlength:%zu", a.length);}, instruction);
			}	
		}

		Text("--motes--");

		for (auto [key,mj] : p.mottes) {
			if (std::holds_alternative<puppet::motte>(mj)) {
				const puppet::motte& i = get<puppet::motte>(mj);
				Text("motte");
				Text("name: %s", key.c_str());
				Text("gravity: %f,%f", i.gravity.x, i.gravity.y);
				Text("friction: %f", i.friction);
				for (auto j : i.ties) {
					Text("tied_to: %s, %f, %f, %f EOL",
						j.target.c_str(), j.pull_length, j.power, j.max_length);
				}
				for (auto j : i.pushes) {
					Text("push_to: %s, %f, %f, %f EOL",
						j.target.c_str(), j.push_length, j.power, j.min_length);
				}
				Text("--");
			} else if (std::holds_alternative<puppet::joint>(mj)) {
				const puppet::joint& i = get<puppet::joint>(mj);
				Text("joint");
				Text("name: %s", key.c_str());
				if (i.fixed_to) Text("fixed_to: %s", i.fixed_to->c_str());
				if (i.elbow_ik) {
					Text("len:%f, from %s to %s", i.elbow_ik->length, i.elbow_ik->shoulder.c_str(), i.elbow_ik->hand.c_str());
				}
			}
		}
		ImGui::End();
	}
}

}
// vim: ts=2 sw=2
