#pragma once

#include "game.h"
#include <filesystem>

//some global variables which record the window height and width
extern int screen_height;
extern int screen_width;

void render_init(const std::filesystem::path&);

void render_draw(const render_world&);
