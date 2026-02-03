#include "./textures.h"
#include <filesystem>
#include <SDL_opengl.h>

#include <string_view>
#include <unordered_map>
#include <print>
#include "filelayer.h"


using std::string;
using std::unordered_map;
using std::println;
namespace fs = std::filesystem;
static unordered_map<fs::path, GLuint> texture_pool;
///put in the primary texture id and its corresponding auxiliary
static unordered_map<GLuint, GLuint> texture2auxiliary;

static bool initialized = false;
const static std::string default_auxiliary = "default_auxiliary";

static bool check_initialized() {
	if (!initialized) println(stderr,
		"developer please call texture_init() sometime before using auiliary_texture");
	return initialized;
}


#ifndef MOCK_TEXTURE_LOADING
GLuint load_texture(const std::filesystem::path& non_con_file) {
	const fs::path file = filelayer::canonise_path(non_con_file);
	SDL_Surface* img = IMG_Load(file.string().c_str());
	if (NULL == img) {
		println(stderr, "Loading File failed. Sorry: {}", file.string());
		return 0;
	}

	//make consistant format
	if (img->format->format != SDL_PIXELFORMAT_ABGR8888) {
		SDL_Surface* fixed_img = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_ABGR8888, 0);  
		SDL_FreeSurface(img);
		img = fixed_img;
	}

	//make new texture
	GLuint texObj = 0;
	glGenTextures(1, &texObj);
	glBindTexture(GL_TEXTURE_2D, texObj);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->w, img->h, 
			0, GL_RGBA, GL_UNSIGNED_BYTE, 
				img->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glGenerateMipmap(GL_TEXTURE_2D);

	texture_pool[file] = texObj;

	SDL_FreeSurface(img);

	return texObj;
}
#else 
GLuint load_texture(const std::filesystem::path& file) {
	static int mock_texture_ids = 0;
	return mock_texture_ids++; 
}
#endif

const static std::string auxiliary_file_suffix = "-auxiliary";
template<class T>
static fs::path potential_auxiliary_path(T primary_path) {
	using namespace std::literals;
	fs::path texture_path = primary_path.string();
	std::string aux_filename = texture_path.stem().string()
		+ auxiliary_file_suffix + texture_path.extension().string();
	fs::path auxiliary_path = texture_path.parent_path() / aux_filename;
	return auxiliary_path;
}

#ifndef MOCK_TEXTURE_LOADING
GLuint get_auxilary_texture(const std::filesystem::path& non_con_path) {
	const fs::path path = filelayer::canonise_path(non_con_path);
	auto potential_path = potential_auxiliary_path(path);	
	if (texture_pool.contains(potential_path)) {
		return texture_pool[potential_path];
	} else if (fs::exists(potential_path)) {
		//luckily loading textures is an idempotent 
		//so we can just load it now
		texture_pool[potential_path] = load_texture(potential_path);
		return texture_pool[potential_path];
	} else {
		return texture_pool[default_auxiliary];
	}
}
#else
GLuint get_auxilary_texture(const std::filesystem::path& non_con_path) {
	return texture_pool[default_auxiliary];
}
#endif

#ifndef MOCK_TEXTURE_LOADING
GLuint get_auxilary_texture(GLuint tex) {
	return texture2auxiliary.at(tex);
}
#else
GLuint get_auxilary_texture(GLuint tex) {
	return texture_pool[default_auxiliary];
}
#endif

#ifndef MOCK_TEXTURE_LOADING
GLuint get_texture(const std::filesystem::path& non_con_path) {
	const fs::path path = filelayer::canonise_path(non_con_path);
	if (!texture_pool.contains(path)) println(stderr, "could not find texture: '{}'\n", path.string());
	return texture_pool[path]; 
}
#else
static int test_counter = 0;
GLuint get_texture(const std::filesystem::path& non_con_path) {
	const fs::path path = filelayer::canonise_path(non_con_path);
	if (!texture_pool.contains(path)) {
		texture_pool[path] = test_counter++;
	}
	return texture_pool[path]; 
}
#endif

void load_textures(const std::filesystem::path& non_con_path) {
	check_initialized();
	const fs::path path = filelayer::canonise_path(non_con_path);
	for (auto& i:  std::filesystem::directory_iterator(path)) {
		std::string ext = i.path().extension().string();
		if (ext == ".png" || ext == ".PNG") {
			GLuint this_tex = load_texture(i.path());
			//associate the corresponding texture with it's auxiliary
			texture2auxiliary[this_tex] = get_auxilary_texture(i.path());
		}
	}
}

void textures_init() {
	texture_pool[default_auxiliary] = load_texture("./ui/default-auxiliary.png");
	IMG_Init(IMG_INIT_PNG);
	initialized = true;
}

#include "imgui.h"
namespace gui {
	void textures_list() {
	    ImGui::Begin("textures info");
			using namespace ImGui;

			if (!ImGui::BeginTable("texture id table", 3, ImGuiTableFlags_SizingStretchProp)) { return;}
			TableNextRow(); TableSetColumnIndex(0); Text("path");
                         TableSetColumnIndex(1); Text("main id");
                         TableSetColumnIndex(2); Text("auxiliary id");


			for (auto [path, main_id] : texture_pool) {
				std::string str = path.string();
				auto auxilary_id = texture2auxiliary[main_id];

				TableNextRow(); TableSetColumnIndex(0); Text("%s", str.c_str());
				                TableSetColumnIndex(1); Text("%d", main_id);  
				                TableSetColumnIndex(2); Text("%d", auxilary_id);  
			}
			ImGui::EndTable();
	    ImGui::End();
					
	}
}

// vim: ts=2 sw=2
