#pragma once
#include <string>
#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#include <SDL_image.h> 
#include <filesystem>

GLuint get_texture(const std::filesystem::path& path);
///find the textureID of the auxiliary texture given the main texture
GLuint get_auxilary_texture(const std::filesystem::path& main_texture_path);
///find the textureID of the auxiliary texture given the auxiliary texture
GLuint get_auxilary_texture(GLuint main_texture);

void load_textures(const std::filesystem::path& path);
void textures_init();

namespace gui {
	void textures_list();
}
