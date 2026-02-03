#version 330 core
layout(location = 0) in vec2 vertex_pos;

//uniform vec2 top; 
//uniform vec2 bottom; 
uniform float width; 
uniform int bendy; 
uniform vec2 angle; 

uniform vec2 noodle_pos[120];
uniform int noodle_length;
uniform mat3 camera_mat;

out vec2 pos;

void main() {
	int index = int( (vertex_pos.y*(noodle_length-1)) );
	
	vec2 axis1;
	vec2 axis2;
	if (bendy == 1) {
		if (index == 0)
			axis1 = noodle_pos[index+1] - noodle_pos[index];
		else if (index == noodle_length-1)
			axis1 = noodle_pos[index] - noodle_pos[index-1];
		else {
			//sum of the other two options
			axis1 =  noodle_pos[index+1] - noodle_pos[index-1];
		}
		axis2 = normalize(cross(vec3(axis1,0), vec3(0,0,1)).xy);
	} else {
		axis2 = normalize(angle);
	}

	vec2 model_space = noodle_pos[index] -axis2*width/2 + axis2*width * vertex_pos.x;
	gl_Position.xyw =  camera_mat * vec3(model_space,1);
	gl_Position.z = 1;

	pos = vertex_pos;
}
