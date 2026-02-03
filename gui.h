#include <SDL.h>
namespace gui {
	void init();
	bool is_gui_event(SDL_Event& event);
	void handle_event(SDL_Event& event);
	void update();
	void shutdown();
}
// vim: ts=2 sw=2
