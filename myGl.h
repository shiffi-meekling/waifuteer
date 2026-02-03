#pragma once
#include <GL/glew.h> 
namespace myGl {
	struct depthFunc {
		GLenum data;

		operator GLenum() const {
			return data;
		}
	};
}
// vim: ts=2 sw=2
