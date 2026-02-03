#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

#include <unordered_map>
#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <unordered_map>

#include "shader.h"
using namespace std;

string readFile(const char * filepath) {
	string fileContents; 
	ifstream file(filepath, ios::in);
	if (file.is_open()) {
		stringstream ss;
		ss << file.rdbuf();
		fileContents = ss.str();
		file.close();
	} else {
		cerr << "error: failed to open file '" << filepath << "'\n";
		return "";
	}
	return fileContents;
}


//returns true on error
bool checkShader(GLuint id) {
	GLint result = GL_FALSE;
	int InfoLogLength;

	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( !result ){
		vector<char> errorMessage(InfoLogLength+1);

		glGetShaderInfoLog(id, InfoLogLength, NULL, &errorMessage[0]);

		printf("shader error: %s\n", &errorMessage[0]);
		return true;
	} else {
		return false;
	}
}
//returns true on error
bool checkProgram(GLuint id) {
	GLint result = GL_FALSE;
	int InfoLogLength;

	glGetProgramiv(id, GL_LINK_STATUS, &result);
	glGetProgramiv(id, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( !result ){
		vector<char> errorMessage(InfoLogLength+1);

		glGetProgramInfoLog(id, InfoLogLength, NULL, &errorMessage[0]);
		cout << ":///" << InfoLogLength << "|" << id << "\n";

		printf("shader linking error: %s\n", &errorMessage[0]);
		return true;
	} else {
		return false;
	}
}

GLuint loadShaders(const char * vertex_file_path, const char * fragment_file_path){

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	string VertexShaderCode2 = readFile(vertex_file_path);
	const char * VertexShaderCode = VertexShaderCode2.c_str();
	// Read the Fragment Shader code from the file
	string FragmentShaderCode2 = readFile(fragment_file_path);
	const char * FragmentShaderCode = FragmentShaderCode2.c_str(); 


	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	
	glShaderSource(VertexShaderID, 1, &VertexShaderCode, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	if (checkShader(VertexShaderID)) return -1;

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	glShaderSource(FragmentShaderID, 1, &FragmentShaderCode, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	if (checkShader(FragmentShaderID)) return -1;

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	if (checkProgram(ProgramID)) return -1;

	// Clean up	
	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}
