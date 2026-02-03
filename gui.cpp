// Dear ImGui: standalone example application for SDL2 + SDL_Renderer
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// Important to understand: SDL_Renderer is an _optional_ component of SDL2.
// For a multi-platform app consider using e.g. SDL+DirectX on Windows and SDL+OpenGL on Linux/OSX.

#include "game.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include "textures.h"
#include <stdio.h>
#include <stdio.h>
#include <SDL.h>
#ifdef _WIN32
#include <windows.h>        // SetProcessDPIAware()
#endif

#include "gui.h"
#include <print>
#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#include <SDL_opengl.h>

#include "textures.h"

namespace gui {

static SDL_Window* window;
static ImGuiIO* io; 
static SDL_GLContext gl_context;

void init() {
		if (window != nullptr) return; //oh the window is already open, so no-op

    // Create window with SDL_Renderer graphics context
    float main_scale = ImGui_ImplSDL2_GetContentScaleForDisplay(0);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL |  SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    window = SDL_CreateWindow("Dear ImGui SDL2+SDL_Renderer example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
    }
//    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
 //   if (renderer == nullptr)
  //  {
   //     SDL_Log("Error creating SDL_Renderer!");
    //}
    //SDL_RendererInfo info;
    //SDL_GetRendererInfo(renderer, &info);
    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
    }

		if (SDL_GL_MakeCurrent(window, gl_context) < 0) {
				std::println("gui setting gl context to gui window failed");
		}
    SDL_GL_SetSwapInterval(1); // Enable vsync
    //SDL_Log("Current SDL_Renderer: %s", info.name);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = &ImGui::GetIO(); 
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (using io->ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    // Setup Platform/Renderer backends
 //   ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
//    ImGui_ImplSDLRenderer2_Init(renderer);
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    const char* glsl_version = "#version 130";
    ImGui_ImplOpenGL3_Init(glsl_version);

}
// Our state
static bool show_game_window = false;
static bool show_texture_window = false;
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
static std::array<std::tuple<std::string, void (*)(), bool>, 4> windows = {{
	{"live data floats", game::live_float, false }, 
	{"live data phys", game::live_phys, false}, 
	{"texture ids", textures_list, false},
	{"puppet", game::parsed_puppet, false},
}};

bool is_gui_event(SDL_Event& event) {
	//I suspect this pulls this
	static bool have_focus = false;
	if ( event.type == SDL_WINDOWEVENT ) {
		if (SDL_GetWindowFromID(event.window.windowID) == window) {
			switch (event.window.event) {
				//Window appeared
				//Mouse enter
				case SDL_WINDOWEVENT_ENTER:
				have_focus = true;
				break;

				//Mouse exit
				case SDL_WINDOWEVENT_LEAVE:
				have_focus = false;
				break;

				//Keyboard focus gained
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				have_focus = true;
				break;

				//Keyboard focus lost
				case SDL_WINDOWEVENT_FOCUS_LOST:
				have_focus = false;
				break;

				//Hide on close
				case SDL_WINDOWEVENT_CLOSE:
					shutdown();
				have_focus = false;
				break;
			}
		}
	}
	return have_focus;
}

void handle_event(SDL_Event& event) {

	ImGui_ImplSDL2_ProcessEvent(&event);
	if (event.type == SDL_WINDOWEVENT 
		&& event.window.event == SDL_WINDOWEVENT_CLOSE 
		&& event.window.windowID == SDL_GetWindowID(window)) shutdown();

}

void update() {
	if (window == nullptr) return;
	SDL_GL_MakeCurrent(window, gl_context);

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	{
	    static float f = 0.0f;
	    static int counter = 0;

	    ImGui::Begin("Hello, world!");   
	    ImGui::Text("This is some useful text."); 
			for (auto& [label, function, b] : windows) {
				ImGui::Checkbox(label.c_str(), &b); 
			}
	    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);  
	    ImGui::ColorEdit3("clear color", (float*)&clear_color);
	    if (ImGui::Button("Button")) counter++; 
	    ImGui::SameLine();
	    ImGui::Text("counter = %d", counter);

	    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
	    ImGui::End();
	}

	// render debug windows
	for (auto& [label, function, b] : windows) {
		if (b) function();
	}

	// Rendering
	ImGui::Render();
	glViewport(0, 0, (int)io->DisplaySize.x, (int)io->DisplaySize.y);
	glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(window);

}

void shutdown() {
		if (window == nullptr) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    window = nullptr;
}

}
// vim: ts=2 sw=2
