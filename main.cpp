#include <SDL.h>
#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#include <SDL_opengl.h>

#include <unordered_map>
#include <cstdlib>

#include "SDL_events.h"
#include "render.h"
#include "game.h"
#include "sound.h"
#include "favicon.h"
#include <print>
#include "face_tracking.h"
#include "edit-notify.h"
#include <boost/program_options.hpp>
#include "gui.h"
#include <format>

using namespace std;
namespace po = boost::program_options;

static void framerate_limit(unsigned int maxFPS) {
	unsigned int frame_ms_budget = 1000 / maxFPS;
	static unsigned long long previousTime = SDL_GetTicks64();
	unsigned long long now = SDL_GetTicks64(); 
	unsigned int actual_ms_used = previousTime - now;
	if (actual_ms_used <= frame_ms_budget) {
		SDL_Delay(frame_ms_budget - actual_ms_used);	
	} else {
		std::println("frame dropped. overbudget {}ms-{}ms={}ms", 
			actual_ms_used, frame_ms_budget, actual_ms_used - frame_ms_budget);	
	}
	previousTime = now;
}

const bool joystick_enabled = true;
static unordered_map<SDL_Keycode, float> keyboard;
static unordered_map<Uint8, float> joystick_buttons;
static mouse_input mouse;

const unordered_map<string, pair<int,int>> resolutions = {
	{"720p", {1280, 720}},
	{"1080p", {1920, 1080}},
	{"shorts", {1080, 1920}},
};



int main(int paramater_count, char** paramaters) {
	po::options_description cmd_desc("Allowed options");
	cmd_desc.add_options()
			("help,h", "produce help message")
			("puppet", po::value<std::filesystem::path>()->default_value("./kantan-chan/"),
				"location of the puppet (model) to display")
			("resolution,r", po::value<string>()->default_value("720p"), "name of the resolution you want to use")
	;
	po::positional_options_description positional_arg;
	positional_arg.add("puppet", 1);

	po::variables_map cmd_vars;        
	po::store(po::command_line_parser(paramater_count, paramaters).
						options(cmd_desc).positional(positional_arg).run(), cmd_vars);
	po::notify(cmd_vars);    

	if (cmd_vars.count("help")) {
			cout << cmd_desc << "\n";
			return 0;
	}

	std::string resolution = cmd_vars["resolution"].as<std::string>();
	screen_width = resolutions.at(resolution).first;
	screen_height = resolutions.at(resolution).second;

	//listen to the gamepad/joystick in the background
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

	//open the window
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		 printf("SDL_Init failed: %s\n", SDL_GetError());
		 SDL_ClearError();
	}
	
	//Decalre that we are DPI Aware
#ifdef _WIN32
    ::SetProcessDPIAware();
#endif
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	auto gWindow = SDL_CreateWindow( "Waifuteer",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		screen_width, screen_height, SDL_WINDOW_OPENGL);
	if (!gWindow) {
		cerr << "Oh no! no window made!";
	}

	load_favicon(gWindow);
	auto context = SDL_GL_CreateContext(gWindow);
	if (!context) {
		cerr << "Oh no! no GL context";
	}
	SDL_GL_MakeCurrent(gWindow, context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	render_init((cmd_vars["puppet"].as<std::filesystem::path>()).c_str());

	sound_init();

	//joystick init;
	SDL_Joystick* joystick;

	if (joystick_enabled) {
		if (SDL_NumJoysticks() > 0) {
						joystick = SDL_JoystickOpen(0);
						SDL_JoystickEventState(SDL_ENABLE);
		} else {
						cerr << "no joystick found\n";
	 }
	}

	tracking::init();
	init_game((cmd_vars["puppet"].as<std::filesystem::path>() / "puppet.pup").c_str());


	//main game loop
	while (1) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
				// gui::is_gui_event is almost certainly bugged 
				if (gui::is_gui_event(event)) {
					gui::handle_event(event);
				} else {
					/* handle your event here */
					if (event.type == SDL_QUIT) {
						goto exit;
					} else if (event.type == SDL_MOUSEMOTION) {
						const double half_width = screen_width/2.0;
						const double half_height = screen_height/2.0;

						mouse.x = (event.motion.x)/half_width - 1;
						mouse.y = (-event.motion.y)/half_height + 1;
					} else if (event.type == SDL_KEYDOWN) {
						keyboard[event.key.keysym.sym] = 1;
						keyboard_events(event);
						if (event.key.keysym.sym == SDLK_F1 || event.key.keysym.sym == SDLK_F12) {
							gui::init();
						}

					} else if (event.type == SDL_KEYUP) {
						//press left control while key is down to stick it down
						if (!keyboard[SDLK_LCTRL] || event.key.keysym.sym == SDLK_LCTRL) {

							keyboard[event.key.keysym.sym] = 0;
						}
					} else if (joystick_enabled && event.type == SDL_JOYBUTTONDOWN) {
						std::cout << "jbutton:" << int(event.jbutton.button) << "\n" ;
						joystick_buttons[event.jbutton.button] = 1;
					} else if (event.type == SDL_JOYBUTTONUP) {
						joystick_buttons[event.jbutton.button] = 0;

					} else if (joystick_enabled && event.type == SDL_MOUSEBUTTONDOWN
								&& event.button.button == SDL_BUTTON_LEFT) {
						mouse.button_left = 1;
					} else if (event.type == SDL_MOUSEBUTTONUP
								&& event.button.button == SDL_BUTTON_LEFT) {
						mouse.button_left = 0;
					} else if (event.type == SDL_MOUSEBUTTONDOWN
								&& event.button.button == SDL_BUTTON_RIGHT) {
						mouse.button_right = 1;
					} else if (event.type == SDL_MOUSEBUTTONUP
								&& event.button.button == SDL_BUTTON_RIGHT) {
						mouse.button_right = 0;
					} else if (event.type == SDL_MOUSEBUTTONDOWN
								&& event.button.button == SDL_BUTTON_MIDDLE) {
						mouse.button_middle = 1;
					} else if (event.type == SDL_MOUSEBUTTONUP
								&& event.button.button == SDL_BUTTON_MIDDLE) {
						mouse.button_middle = 0;
					} else if (event.type == SDL_MOUSEBUTTONDOWN
								&& event.button.button == SDL_BUTTON_X1) {
						mouse.button_x1 = 1;
					} else if (event.type == SDL_MOUSEBUTTONUP
								&& event.button.button == SDL_BUTTON_X1) {
						mouse.button_x2 = 0;
					} else if (event.type == SDL_MOUSEBUTTONDOWN
								&& event.button.button == SDL_BUTTON_X2) {
						mouse.button_x2 = 1;
					} else if (event.type == SDL_MOUSEBUTTONUP
								&& event.button.button == SDL_BUTTON_X2) {
						mouse.button_x2 = 0;
					}
			}

		}
		
		//get the tracking data
		tracking::new_frame();
		std::optional<tracking::metrics> maybe_metrics = tracking::get_metrics();	
		tracking::metrics metrics = maybe_metrics ? *maybe_metrics : tracking::metrics{};	

		SDL_GL_MakeCurrent(gWindow, context);
		render_draw(update_gamestate(mouse, keyboard,
			joystick_buttons, metrics, get_sound_level()));
		SDL_GL_SwapWindow(gWindow);

		//framerate_limit(30);
		edit_notify::poll();
		gui::update();
	}

exit:
	gui::shutdown();
	SDL_Quit();
	if (joystick_enabled) SDL_JoystickClose(joystick);
  return 0;
}

// vim: sw=2 ts=2
