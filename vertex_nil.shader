#version 330 core
layout(location = 0) in vec2 vertex_pos;

out vec2 pos;


void main() {
	gl_Position.xy = vertex_pos;  
	gl_Position.zw = vec2(1,1);

	pos = vertex_pos;
}
