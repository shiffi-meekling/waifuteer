#pragma once
#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#include "vec.h"
#include <vector>
#include "myGl.h"
#include <format>

///all the textures which work together to display one thing 
struct material {
	///the direct colors which are visible
	GLuint texture;
	///the other information, like depth, jiggle-maps
	GLuint auxiliary;
};

///normally would be called a ribbon in 3D rendering I think?
struct noodle {
	struct material material;
	std::vector<vec2> points;
	float width;
	float back;
	float front;
	myGl::depthFunc depthFunc;
	vec3 whiteColor;
	bool bendy;
	vec2 angle;
};

struct renderables {
	std::vector<noodle> noodles;
};
// vim: ts=2 sw=2
