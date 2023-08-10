#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
using namespace std;

#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>

#include "Shadinclude.hpp"
#include "shader.hpp"

const int debugLevel = 0;

//struct RetVal {
//	GLuint frVeID;
//	GLuint compID;
//};

// ShProgs LoadShaders(const char * vertex_file_path,const char * fragment_file_path, const char* compute_file_path1, const char* compute_file_path2){

// 	// Create the shaders
// 	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
// 	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
// 	GLuint ComputeShaderID1 = glCreateShader(GL_COMPUTE_SHADER);
// 	GLuint ComputeShaderID2 = glCreateShader(GL_COMPUTE_SHADER);

// 	// Read the Vertex Shader code from the file
// 	std::string VertexShaderCode;
// 	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
// 	if(VertexShaderStream.is_open()){
// 		std::stringstream sstr;
// 		sstr << VertexShaderStream.rdbuf();
// 		VertexShaderCode = sstr.str();
// 		VertexShaderStream.close();
// 	}else{
// 		printf("Impossible to open %s.\n", vertex_file_path);
// 		getchar();
// 		return { 0,0 };
// 	}

// 	// Read the Fragment Shader code from the file
// 	std::string FragmentShaderCode;
// 	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
// 	if(FragmentShaderStream.is_open()){
// 		std::stringstream sstr;
// 		sstr << FragmentShaderStream.rdbuf();
// 		FragmentShaderCode = sstr.str();
// 		FragmentShaderStream.close();
// 	}
// 	else {
// 		printf("Impossible to open %s !\n", fragment_file_path);
// 		getchar();
// 		return { 0,0 };
// 	}

// 	// Read the Compute Shader code from the file1
// 	std::string ComputeShaderCode1;
// 	std::ifstream ComputeShaderStream1(compute_file_path1, std::ios::in);
// 	if (ComputeShaderStream1.is_open()) {
// 		std::stringstream sstr;
// 		sstr << ComputeShaderStream1.rdbuf();
// 		//replace(sstr,"$sizeX", "")
// 		ComputeShaderCode1 = sstr.str();
// 		ComputeShaderStream1.close();
// 	}
// 	else {
// 		printf("Impossible to open %s. \n", compute_file_path1);
// 		getchar();
// 		return { 0,0 };
// 	}

// 	// Read the Compute Shader code from the file2
// 	std::string ComputeShaderCode2;
// 	std::ifstream ComputeShaderStream2(compute_file_path2, std::ios::in);
// 	if (ComputeShaderStream2.is_open()) {
// 		std::stringstream sstr;
// 		sstr << ComputeShaderStream2.rdbuf();
// 		//replace(sstr,"$sizeX", "")
// 		ComputeShaderCode2 = sstr.str();
// 		ComputeShaderStream2.close();
// 	}
// 	else {
// 		printf("Impossible to open %s. \n", compute_file_path2);
// 		getchar();
// 		return { 0,0 };
// 	}

// 	GLint Result = GL_FALSE;
// 	int InfoLogLength;

// 	// Compile Vertex Shader
// 	printf("Compiling shader : %s\n", vertex_file_path);
// 	char const * VertexSourcePointer = VertexShaderCode.c_str();
// 	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
// 	glCompileShader(VertexShaderID);

// 	// Check Vertex Shader
// 	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
// 	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
// 	if ( InfoLogLength > 0 ){
// 		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
// 		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
// 		printf("%s\n", &VertexShaderErrorMessage[0]);
// 	}

// 	// Compile Fragment Shader
// 	printf("Compiling shader : %s\n", fragment_file_path);
// 	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
// 	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
// 	glCompileShader(FragmentShaderID);

// 	// Check Fragment Shader
// 	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
// 	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
// 	if ( InfoLogLength > 0 ){
// 		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
// 		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
// 		printf("%s\n", &FragmentShaderErrorMessage[0]);
// 	}

// 	// Compile Compute Shader1
// 	printf("Compiling shader : %s\n", compute_file_path1);
// 	char const* ComputeSourcePointer1 = ComputeShaderCode1.c_str();
// 	glShaderSource(ComputeShaderID1, 1, &ComputeSourcePointer1, NULL);
// 	glCompileShader(ComputeShaderID1);

// 	// Check Compute Shader
// 	glGetShaderiv(ComputeShaderID1, GL_COMPILE_STATUS, &Result);
// 	glGetShaderiv(ComputeShaderID1, GL_INFO_LOG_LENGTH, &InfoLogLength);
// 	if (InfoLogLength > 0) {
// 		std::vector<char> ComputeShaderErrorMessage(InfoLogLength + 1);
// 		glGetShaderInfoLog(ComputeShaderID1, InfoLogLength, NULL, &ComputeShaderErrorMessage[0]);
// 		printf("%s\n", &ComputeShaderErrorMessage[0]);
// 	}

// 	// Compile Compute Shader2
// 	printf("Compiling shader : %s\n", compute_file_path2);
// 	char const* ComputeSourcePointer2 = ComputeShaderCode2.c_str();
// 	glShaderSource(ComputeShaderID2, 1, &ComputeSourcePointer2, NULL);
// 	glCompileShader(ComputeShaderID2);

// 	// Check Compute Shader
// 	glGetShaderiv(ComputeShaderID2, GL_COMPILE_STATUS, &Result);
// 	glGetShaderiv(ComputeShaderID2, GL_INFO_LOG_LENGTH, &InfoLogLength);
// 	if (InfoLogLength > 0) {
// 		std::vector<char> ComputeShaderErrorMessage(InfoLogLength + 1);
// 		glGetShaderInfoLog(ComputeShaderID2, InfoLogLength, NULL, &ComputeShaderErrorMessage[0]);
// 		printf("%s\n", &ComputeShaderErrorMessage[0]);
// 	}

// 	// Link the program
// 	printf("Linking program\n");
// 	GLuint ProgramID1 = glCreateProgram();
// 	GLuint ProgramID2 = glCreateProgram();
// 	GLuint ProgramID3 = glCreateProgram();
// 	glAttachShader(ProgramID1, VertexShaderID);
// 	glAttachShader(ProgramID1, FragmentShaderID);
// 	glAttachShader(ProgramID2, ComputeShaderID1);
// 	glAttachShader(ProgramID3, ComputeShaderID2);
// 	glLinkProgram(ProgramID1);
// 	glLinkProgram(ProgramID2);
// 	glLinkProgram(ProgramID3);

// 	// Check the program
// 	glGetProgramiv(ProgramID1, GL_LINK_STATUS, &Result);
// 	glGetProgramiv(ProgramID1, GL_INFO_LOG_LENGTH, &InfoLogLength);
// 	if ( InfoLogLength > 0 ){
// 		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
// 		glGetProgramInfoLog(ProgramID1, InfoLogLength, NULL, &ProgramErrorMessage[0]);
// 		printf("%s\n", &ProgramErrorMessage[0]);
// 	}

// 	// Check the program
// 	glGetProgramiv(ProgramID2, GL_LINK_STATUS, &Result);
// 	glGetProgramiv(ProgramID2, GL_INFO_LOG_LENGTH, &InfoLogLength);
// 	if (InfoLogLength > 0) {
// 		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
// 		glGetProgramInfoLog(ProgramID2, InfoLogLength, NULL, &ProgramErrorMessage[0]);
// 		printf("%s\n", &ProgramErrorMessage[0]);
// 	}

// 	// Check the program
// 	glGetProgramiv(ProgramID3, GL_LINK_STATUS, &Result);
// 	glGetProgramiv(ProgramID3, GL_INFO_LOG_LENGTH, &InfoLogLength);
// 	if (InfoLogLength > 0) {
// 		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
// 		glGetProgramInfoLog(ProgramID3, InfoLogLength, NULL, &ProgramErrorMessage[0]);
// 		printf("%s\n", &ProgramErrorMessage[0]);
// 	}

// 	glDetachShader(ProgramID1, VertexShaderID);
// 	glDetachShader(ProgramID1, FragmentShaderID);
// 	glDetachShader(ProgramID2, ComputeShaderID1);
// 	glDetachShader(ProgramID3, ComputeShaderID2);

// 	glDeleteShader(VertexShaderID);
// 	glDeleteShader(FragmentShaderID);
// 	glDeleteShader(ComputeShaderID1);
// 	glDeleteShader(ComputeShaderID2);

// 	return { ProgramID1, ProgramID2, ProgramID3 };
// }

bool replace(std::string &str, const std::string &from, const std::string &to)
{
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

GLuint LoadComputeShader(const char *compute_file_path, int workGroupSizeX, int workGroupSizeY)
{
	GLuint ComputeShaderID = glCreateShader(GL_COMPUTE_SHADER);
	// Read the Compute Shader code from the file1
	std::string ComputeShaderCode;
	// std::ifstream ComputeShaderStream(compute_file_path, std::ios::in);
	// if (ComputeShaderStream.is_open()) {
	// 	std::stringstream sstr;
	// 	sstr << ComputeShaderStream.rdbuf();
	// 	// ComputeShaderCode = sstr.str();
	// 	ComputeShaderStream.close();
	// 	ComputeShaderCode.clear();
	ComputeShaderCode = Shadinclude::load(compute_file_path);
	int blockSizeX = workGroupSizeX;
	int blockSizeY = workGroupSizeY;
	//X
	string parName = std::string("BLOCK_SIZE_X");
	int pos = ComputeShaderCode.find(parName);
	//parName.push_back((char)(4 + '0'));
	//string costring(char(blockSizeX + '0'))
	// prideda par reiksme prie galo, +'0', nes ASCII simoliai nuo '0'
	ComputeShaderCode.replace(pos + parName.length() + 1, 1, " ");					  //istrinam "1" is #define ... 1
	ComputeShaderCode.insert(pos + parName.length() + 1, std::to_string(blockSizeX)); //+1 nes tarpas
	//Y
	parName = std::string("BLOCK_SIZE_Y");
	pos = ComputeShaderCode.find(parName);
	ComputeShaderCode.replace(pos + parName.length() + 1, 1, " ");					  //istrinam "1" is #define ... 1
	ComputeShaderCode.insert(pos + parName.length() + 1, std::to_string(blockSizeY)); //+1 nes tarpas
	// cout << ComputeShaderCode << endl;
	//len = parName.length() + 2;
	//		ComputeShaderCode.replace(pos, len, "BLOCK_SIZE_Y 4");
	// }
	// else {
	// 	printf("Impossible to open %s. \n", compute_file_path);
	// 	getchar();
	// 	return 0;
	// }
	GLint Result = GL_FALSE;
	int InfoLogLength;
	// Compile Compute Shader
	if (debugLevel > 0)
		printf("\nCompiling shader : %s\n", compute_file_path);
	char const *ComputeSourcePointer = ComputeShaderCode.c_str();
	glShaderSource(ComputeShaderID, 1, &ComputeSourcePointer, NULL);
	glCompileShader(ComputeShaderID);
	// Check Compute Shader
	glGetShaderiv(ComputeShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(ComputeShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> ComputeShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(ComputeShaderID, InfoLogLength, NULL, &ComputeShaderErrorMessage[0]);
		printf("%s", &ComputeShaderErrorMessage[0]);
		// exit(-1);
	}
	// Link the program
	if (debugLevel > 0)
		printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, ComputeShaderID);
	glLinkProgram(ProgramID);
	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s", &ProgramErrorMessage[0]);
		// exit(-1);
	}

	glDetachShader(ProgramID, ComputeShaderID);

	glDeleteShader(ComputeShaderID);

	return ProgramID;
}

GLuint LoadVertexFragmentShaders(const char *vertex_file_path, const char *fragment_file_path)
{
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open())
	{
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else
	{
		printf("Impossible to open %s.\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open())
	{
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}
	else
	{
		printf("Impossible to open %s !\n", fragment_file_path);
		getchar();
		return 0;
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	if (debugLevel > 0)
		printf("\nCompiling shader : %s\n", vertex_file_path);
	char const *VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	if (debugLevel > 0)
		printf("Compiling shader : %s\n", fragment_file_path);
	char const *FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	if (debugLevel > 0)
		printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

//// Read config.txt, where workgroup size is stored and replace shader with these values
//std::string configFileTxt;
//std::ifstream configFileStream("config.txt", std::ios::in);
//if (configFileStream.is_open()) {
//	std::stringstream sstr2;
//	sstr2 << configFileStream.rdbuf();
//	configFileTxt = sstr2.str();
//	configFileStream.close();
//	int blockSizeX = 0;
//	int blockSizeY = 0;
//	blockSizeX = stoi(configFileTxt.substr(0, 1));
//	blockSizeY = stoi(configFileTxt.substr(0, 1));
//
//	string parName = std::string("BLOCK_SIZE_X");
//	int pos = ComputeShaderCode.find(parName);
//	int len = parName.length() + 2;
//	//parName.push_back((char)(4 + '0'));
//	ComputeShaderCode.replace(pos, len, "BLOCK_SIZE_X 4"); // prideda par reiksme prie galo, +'0', nes ASCII simoliai nuo '0'
//	parName = std::string("BLOCK_SIZE_Y");
//	pos = ComputeShaderCode.find(parName);
//	len = parName.length() + 2;
//	ComputeShaderCode.replace(pos, len, "BLOCK_SIZE_Y 4");
//}
//else {
//	printf("Impossible to open config.txt. Default values are used. \n");
//}