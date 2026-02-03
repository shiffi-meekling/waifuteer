#include <iostream>

#include <SDL.h>
#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <print>

#include "render.h"


#include "face_tracking.h"
#include "shader.h"
#include "vec.h"
#include "rect.h"
#include "game.h"
#include "textures.h"
#include "filelayer.h"


int screen_height;
int screen_width;

using namespace std;

#define CASE_GL_ERROR(x) case x : cout << "render gl error:" << #x << "\n"; break;

void checkGlErrors() {
	int error = glGetError();
	switch (error) {
		CASE_GL_ERROR(GL_INVALID_ENUM);
		CASE_GL_ERROR(GL_INVALID_VALUE);
		CASE_GL_ERROR(GL_INVALID_OPERATION);
		CASE_GL_ERROR(GL_INVALID_FRAMEBUFFER_OPERATION);
		CASE_GL_ERROR(GL_OUT_OF_MEMORY);
		//CASE_GL_ERROR(GL_NO_ERROR);
	}
}

void checkGlDrawbuffer() {
	int error = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	switch (error) {
		CASE_GL_ERROR(GL_FRAMEBUFFER_UNDEFINED);
		CASE_GL_ERROR(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
		CASE_GL_ERROR(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
		CASE_GL_ERROR(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
		CASE_GL_ERROR(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
		CASE_GL_ERROR(GL_FRAMEBUFFER_UNSUPPORTED);
		CASE_GL_ERROR(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
		CASE_GL_ERROR(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);
	}
}

GLuint makeVertexArray(vector<GLfloat> vertex_data) {

	GLuint new_vector_buffer;
	// Generate 1 buffer, put the resulting identifier in new_vector_buffer
	glGenBuffers(1, &new_vector_buffer);
	
	glBindBuffer(GL_ARRAY_BUFFER, new_vector_buffer);
	// Give our vertices to OpenGL.
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*vertex_data.size(),
		   			vertex_data.data(), GL_STATIC_DRAW);
	
	//generate 1 vertexArray 
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);
	return VertexArrayID; 
}

vector<GLfloat> noodle_buffer_data(int length) {
	vector<GLfloat> buffer;
	length--;//decrease length by 1
	//because the below algorithm over allocates by 1

	buffer.reserve(length*4+4); 
	//2 floats per point, 2 points per length plus 2 points to start

	for (int i = 0; i <= length; i++) {
		float y = (float)i / length;
		//point 1
		buffer.push_back(0.0f);
		buffer.push_back(y);
		//point 2
		buffer.push_back(1.0f);
		buffer.push_back(y);
	}
	return buffer;
}

const int flame_height = 30;

static GLuint square_va;
static GLuint flame_va;
static GLuint tenticle_va;
static GLuint bosom_va;
static GLuint post_processing_va;
void initBuffers() {
	const vector<GLfloat> square_buffer_data = noodle_buffer_data(2);
	square_va = makeVertexArray(square_buffer_data); 

	flame_va = makeVertexArray(noodle_buffer_data(flame_height)); 

	bosom_va = makeVertexArray( noodle_buffer_data(3)); 

	const vector<GLfloat> post_process_data = {-1,-1, -1,1, 1,-1, 1,1};
	post_processing_va = makeVertexArray(post_process_data); 
}

GLuint getVertexArray(int length) {
	static std::unordered_map<int, GLuint> vertex_arrays_map;
	if (vertex_arrays_map.contains(length)) {
		return vertex_arrays_map[length];
	} else {
		vertex_arrays_map[length] = makeVertexArray(noodle_buffer_data(length)); 
		return vertex_arrays_map[length];
	}
}

static GLuint mainFramebuffer;
static GLuint bufferTex;
void initFrameBuffers() {
	glGenFramebuffers(1, &mainFramebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mainFramebuffer);

	//generate Texture to render the colors to
	const long unsigned color_mem_length = screen_width*screen_height*4;
	static vector<float> color_mem(color_mem_length);

	glGenTextures(1, &bufferTex);
	glBindTexture(GL_TEXTURE_2D, bufferTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, screen_width, screen_height, 
			0, GL_RGBA, GL_FLOAT, color_mem.data());
	checkGlErrors();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, bufferTex, 0 );
	
	//generate the depth and stencil buffer
	
	GLuint renderBuffer;
	glGenRenderbuffers(1, &renderBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);	
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 0, GL_DEPTH32F_STENCIL8, 
			screen_width, screen_height);

	glFramebufferRenderbuffer( GL_DRAW_FRAMEBUFFER,  GL_DEPTH_STENCIL_ATTACHMENT,  
			GL_RENDERBUFFER, renderBuffer );
	checkGlErrors();

}

//id of our program
static GLint mainProgram;
//main renderer
static GLint postProgram;
//the post-processing renderer

void render_init(const std::filesystem::path& path) {
	std::filesystem::path model_path = filelayer::canonise_path(path);

	//error checking and info
	if (GLEW_OK != glewInit()) cerr << "Oh no! glew failed to init :ccc\n";

	{
		int gl_version;
		cout << "Using GL version: ";
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &gl_version);
		cout << gl_version << '.';
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &gl_version);
		cout << gl_version << '\n';
	}

	auto error = glGetError();
	if (error != GL_NO_ERROR) {
		cerr << "Oh no! Gl error number #" << error << "\n";
	}


	////////
	initBuffers();
	initFrameBuffers();
	//
	// .......... <- salt to keep the ghosts away
	//

	//set up shader
	mainProgram = loadShaders("./vertex.shader", "./frag.shader");
	if (mainProgram == -1) printf("shader failled to load! opsie");

	postProgram = loadShaders("./vertex_nil.shader", "./post.shader");
	if (mainProgram == -1) printf("post processing shader failled to load! opsie");

	glUseProgram(mainProgram);

	//configure clear
	glClearColor(1.0, 1.0, 1.0, 1);
	glClearDepth(0);

	//configure depth test
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_GEQUAL);

	//load Textures
	textures_init();
	std::println("loading from textures path: {}", model_path.string());
	load_textures(model_path);
	load_textures("./backgrounds/");
	load_textures("./ui/");
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );


	//enable transparency
	glEnable( GL_BLEND );
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//enable stencil
	//doesn't work for mysterious reasons	
	glEnable(GL_STENCIL);
	glStencilFunc(GL_ALWAYS, 0xFF, 0xFF);
	glStencilMask(0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);


	//unneeded as this is default but it is a clue if I need more textures	
	//glUniform1i(uniform_locations["tex"], GL_TEXTURE0);
	//now it causes errors if uncommented

}

void set_depth(float back, float front) {
	glUniform1f(glGetUniformLocation(mainProgram, "front"), front);
	glUniform1f(glGetUniformLocation(mainProgram, "back"), back);
}

void set_camera(float offset_x=0, float offset_y=0, float scale_x=1, float scale_y=1) {
	const GLfloat camera_mat[9] = {scale_x,  0, offset_x,
		0, scale_y, offset_y,
		0,       0,        1 };

	glUniformMatrix3fv(glGetUniformLocation(mainProgram, "camera_mat"), 1, true, camera_mat);
}


void draw_noodle(float* data, size_t size, GLuint noodle_va, float width, bool bendy = true, vec2 angle= {0.0, 1.0}) {
	glUniform2fv(glGetUniformLocation(mainProgram, "noodle_pos"), 
			size, data);
	glUniform1i(glGetUniformLocation(mainProgram, "noodle_length"), size);

	glUniform1f(glGetUniformLocation(mainProgram, "width"), width);
	glUniform2f(glGetUniformLocation(mainProgram, "angle"), angle.x, angle.y);
	glUniform1i(glGetUniformLocation(mainProgram, "bendy"),  bendy);
	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, noodle_va);
	//glBindVertexArray(noodle_va);
	glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			2,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
	// Draw the triangle !
	glDrawArrays(GL_TRIANGLE_STRIP, 0, size*2); // Starting from vertex 0, draw all the vertexes. 
	glDisableVertexAttribArray(0);
}

void draw_noodle(vector<vec2> pos, GLuint noodle_va, float width, bool bendy = true, vec2 angle= {0.0, 1.0}) {
	draw_noodle( reinterpret_cast<float*>(pos.data()), pos.size(), noodle_va, width, bendy , angle);
 
}
void draw_noodle(vector<phys> pos, GLuint noodle_va, float width, bool bendy = true, vec2 angle= {0.0, 1.0}) {
	vector<vec2> tmp;
	for (auto i : pos) tmp.push_back(i.p);
	draw_noodle(tmp, noodle_va, width, bendy, angle);
}

void draw_internal(vec2 top, vec2 bottom, float width = 0.6) {
	draw_noodle({bottom, top}, square_va, width);
}

rect overdraw(vec2 top, float top_overdraw, vec2 bottom, float bottom_overdraw, float width) {
	vec2 v = (top - bottom ).norm();

	vec2 top2 = top+v*top_overdraw;
	vec2 bottom2 = bottom-v*bottom_overdraw;
	draw_internal(top2, bottom2, width);

	return {bottom2, top2, width};
}

void debug_draw(const char* texture, vec2 pos) {
	glBindTexture(GL_TEXTURE_2D, get_texture("./ui/face-arrow.png"));
	const float width = 0.1f;
	glUniform3f(glGetUniformLocation(mainProgram, "white"), 1,1,1);
	draw_internal(pos, pos + vec2({0,-width}), width);
}

void render_draw(const render_world& state) {
	glUseProgram(mainProgram);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mainFramebuffer);
	checkGlDrawbuffer() ;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	// Set the uniform values (the integers correspond to texture units)
	glUniform1i(glGetUniformLocation(mainProgram, "tex"), 0); // GL_TEXTURE0
	glUniform1i(glGetUniformLocation(mainProgram, "auxiliary"), 1); // GL_TEXTURE1

	glUniform3f(glGetUniformLocation(mainProgram, "white"), 1.01,1.01,1.01);
	glUniform3f(glGetUniformLocation(mainProgram, "black"), 0,0,0);

	//
	set_camera(0,0,1,1);
	set_depth(0,0);

	/* one day we will have masking, maybe. I never got the stencil to work
	//test stencil
	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, get_texture("./textures/background.png"));
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);

	draw_internal({0, 1}, {0, -1}, 1);

	//tell everyone to use the stencil
	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
	*/

	glActiveTexture(GL_TEXTURE0);
	if (state.background == 0) {
		glClearColor(0/255, 255/255.0, 0/255.0, 1);
	} else if (state.background == 1) {
		glBindTexture(GL_TEXTURE_2D, get_texture("./backgrounds/background.png"));
		draw_internal({0, 1}, {0, -1}, 2);
	} else if (state.background == 2) {
		glClearColor(sin(state.time*0.18)+1, cos(state.time*0.3), 1.0, 1);
	} else if (state.background == 3) {
		glBindTexture(GL_TEXTURE_2D, get_texture("./backgrounds/background2.png"));
		draw_internal({0, 1}, {0, -1}, 2);
	} else if (state.background == 4) {
		glBindTexture(GL_TEXTURE_2D, get_texture("./backgrounds/background3.png"));
		draw_internal({0, 1}, {0, -1}, 2);
	}

	//set_camera(0, 0, state.camera_scale.p.x/state.camera_scale.p.y, 1);

	set_depth(0.001,0.003);
	set_camera(state.camera_pan.p.x, state.camera_pan.p.y, state.camera_scale.p.x, state.camera_scale.p.y);

	/////////////////////
	//aanoodle
	for (const noodle &n : state.render.noodles ) {
		vec3 white = n.whiteColor;
		glUniform3f(glGetUniformLocation(mainProgram, "white"), white.x, white.y, white.z);
		glDepthFunc(n.depthFunc);
		set_depth(n.back, n.front);

		// bind the visible texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, n.material.texture);

		// bind depth map, jiggle map, etc
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, n.material.auxiliary);

		draw_noodle(n.points, getVertexArray(n.points.size()), n.width, n.bendy, n.angle);
	}	
	glActiveTexture(GL_TEXTURE0);
	glDepthFunc(GL_GEQUAL);

	//debug info
	set_depth(1.0, 1.0);
	if (state.debug) {
		debug_draw("./ui/face-arrow.png", state.tracking_data.nose);
	}

	//post processing/////
	glUseProgram(postProgram);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, bufferTex);
	glUniform1i(glGetUniformLocation(postProgram, "sparkle"), state.sparkle);
	glUniform1f(glGetUniformLocation(postProgram, "random_seed"), 
			((state.time >> 4) % 100 + 1) / 99.0); //tinkle
	glUniform1i(glGetUniformLocation(postProgram, "bloom_size"), 
			min(screen_height, screen_width)/50);
	glUniform1f(glGetUniformLocation(postProgram, "pixel_size_x"), 1.0/screen_width);
	glUniform1f(glGetUniformLocation(postProgram, "pixel_size_y"), 1.0/screen_height);


	//draw a full image quad 
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, post_processing_va);
	glVertexAttribPointer(
			0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
			2,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
			);
	// Draw the triangle !
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4*2); // Starting from vertex 0, draw all the vertexes. 
	glDisableVertexAttribArray(0);
}

// vi: ts=2 sw=2 
