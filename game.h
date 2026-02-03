#pragma once

#include <unordered_map>

#include <SDL.h>
#include <filesystem>

#include "renderables.h"
#include "phys.h"
#include "face_tracking.h"

//contains all the information needed to render a frame
struct render_world {
	bool debug;
	unsigned long long time;

	//camera
	phys camera_pan;
	phys camera_scale;

	//included for debugging displays
	tracking::metrics tracking_data;

	int sparkle;
	int background;

	renderables render;
};

struct mouse_input {
	float x;
	float y;
	float button_left;
	float button_middle;
	float button_right;
	float button_x1;
	float button_x2;
};

render_world update_gamestate(mouse_input &mouse,
	   	std::unordered_map<SDL_Keycode, float> &keyboard,
	   	std::unordered_map<Uint8, float> &joystick_buttons,
			tracking::metrics metrics,
			float mic
		) ;

///set the internal game state to something fixed
void init_game(std::filesystem::path puppet_path);
///you must call the full version once first
void init_game();
void keyboard_events(SDL_Event);
// vim: ts=2 sw=2
namespace gui {
	namespace game { 
		void live_phys();
		void live_float();
		void parsed_puppet();
	}
}
