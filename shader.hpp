#ifndef SHADER_HPP
#define SHADER_HPP

struct ShProgs {
	GLuint frVeID;
	GLuint compID1;
	GLuint compID2;
};

bool replace(std::string& str, const std::string& from, const std::string& to);
ShProgs LoadShaders(const char * vertex_file_path,const char * fragment_file_path, const char* compute_file_path1, const char* compute_file_path2);
GLuint LoadComputeShader(const char* compute_file_path, int workGoupSizeX, int workGroupSizeY);
GLuint LoadVertexFragmentShaders(const char* vertex_file_path, const char* fragment_file_path);

#endif
