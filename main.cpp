#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/empty.pb.h>
#include "fentonControl.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::FentonControl;
using helloworld::HelloReply2;
using helloworld::HelloRequest;
using helloworld::VmReply;
// using helloworld::StopSim;

#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <time.h>
#include <Windows.h>
#include <thread>
#include <queue>
#include <mutex>

#include <GL/glew.h>

#include <GLFW/glfw3.h>
GLFWwindow *window;

// #include <glm/glm.hpp>
// using namespace glm;

#include "shader.hpp"

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void RunServer(std::unique_ptr<grpc::Server> *serverPtr);
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity,
							GLsizei length, const char *message, const void *userParam);

bool lbutton_down = false;
bool enableGjModel = true;
bool increaseGj = false;
bool decreaseGj = false;
bool increaseGjZone = false;
bool decreaseGjZone = false;
bool increaseSensitivity = false;
bool decreaseSensitivity = false;
bool increaseAnis = false;
bool decreaseAnis = false;

std::mutex mtx;
std::unique_ptr<grpc::Server> *serverPtr;

// grpc control interface
bool pause = false;

clock_t t_start, t_end;

using namespace std;

queue<int> queueCmd;
queue<float *> queueReply;

const unsigned int sizeX = 64 + 1; //*3
const unsigned int sizeY = 64 + 1;
const int workGroupSize = 32; //32
const int itersInFrame = 20;
// Open a window and create its OpenGL context
int scale = 600;
int width = scale * sizeX / sizeY; //*2 kai 2 2d grafikai
int height = scale;

// size for ssb and vertex draw
const int i_frame_max = 100000;
//Shader storege buffer. For compute shader
float *ssb_u_host = new float[sizeX * sizeY];
float *ssb_v_host = new float[sizeX * sizeY];
float *ssb_w_host = new float[sizeX * sizeY];
float *ssb_J_ion_host = new float[sizeX * sizeY];
float *ssb_J_gj_host = new float[sizeX * sizeY * 3]; // *3: W,NW,NE
float *ssb_u_reg_host = new float[i_frame_max];
float *ssb_gj_host = new float[sizeX * sizeY * 3]; // *3: W,NW,NE
float *ssb_gj_reg_host = new float[i_frame_max * 2];
float *ssb_gj_par_host = new float[sizeX * sizeY * 3 * 7 * 2]; // *3: W,NW,NE; *7: parameters count; *2:gates count
float *ssb_gj_p_host = new float[sizeX * sizeY * 3 * 4];	   // markov state matices for each gj
float *ssb_dbg_host = new float[i_frame_max * 3];

GLuint ssbo_u, ssbo_v, ssbo_w, ssbo_J_ion, ssbo_J_gj, ssbo_u_reg, ssbo_gj, ssbo_gj_reg, ssbo_gj_par, ssbo_gj_p, ssbo_dbg;

int main(void)
{
	cout << "Starting..\n";

	HWND consoleWindow = GetConsoleWindow();

	SetWindowPos(consoleWindow, 0, 0, 500, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	std::thread serverThread(RunServer, serverPtr); //TODO: clean server shutdown

	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

	window = glfwCreateWindow(width, height, "Playground", NULL, NULL);
	if (window == NULL)
	{
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwSetWindowPos(window, 0, 0);
	glfwMakeContextCurrent(window);

	cout << "\nGPU: " << glGetString(GL_VENDOR) << ", " << glGetString(GL_RENDERER) << endl;

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		// initialize debug output
		cout << "Initialize debug output.\n";
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		// glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		glDebugMessageControl(GL_DEBUG_SOURCE_API,
							  GL_DEBUG_TYPE_ERROR,
							  GL_DEBUG_SEVERITY_HIGH,
							  0, nullptr, GL_TRUE);
		cout << "Initialize debug output. Done.\n";
	}

	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Create and compile our GLSL program from the shaders
	//ShProgs shIDs = LoadShaders("SimpleVertexShader.vert", "SimpleFragmentShader.frag", "compShader1.comp", "compShader2.comp");
	GLuint kernel1 = LoadComputeShader("compShader1.comp", workGroupSize);
	GLuint kernel2 = LoadComputeShader("compShader2.comp", workGroupSize);
	GLuint vertFragShadersID = LoadVertexFragmentShaders("SimpleVertexShader.vert", "SimpleFragmentShader.frag");
	GLuint line2DvertFragShadersID = LoadVertexFragmentShaders("linePlot.vert", "linePlot.frag");

	for (int x = 0; x < sizeX; x++)
		for (int y = 0; y < sizeY; y++)
		{
			ssb_u_host[y * sizeX + x] = 0;	// normal
			ssb_v_host[y * sizeX + x] = 1.; //0.999970794;
			ssb_w_host[y * sizeX + x] = 1.; //0.999938905;
			// ssb_u_host[y * sizeX + x] = x <= 128 && y <= 64 - 8 ? 1. : 0; // rotor bandymai
			// ssb_v_host[y * sizeX + x] = x <= 128 ? 0. : 1.; //0.999970794;
			// ssb_w_host[y * sizeX + x] = x <= 128 ? 0. : 1.; //0.999938905;
			ssb_J_ion_host[y * sizeX + x] = 0;
			ssb_J_gj_host[y * sizeX * 3 + x * 3] = 0.f;		//W
			ssb_J_gj_host[y * sizeX * 3 + x * 3 + 1] = 0.f; //NW
			ssb_J_gj_host[y * sizeX * 3 + x * 3 + 2] = 0.f; //NE
			// ssb_gj_host[y * sizeX * 3 + x * 3] = .2e-6f; //W
			// ssb_gj_host[y * sizeX * 3 + x * 3 + 1] = .05e-6f; //NW
			// ssb_gj_host[y * sizeX * 3 + x * 3 + 2] = .05e-6f; //NE
			ssb_gj_host[y * sizeX * 3 + x * 3] = 4e-8f;		//W
			ssb_gj_host[y * sizeX * 3 + x * 3 + 1] = 4e-8f; //NW
			ssb_gj_host[y * sizeX * 3 + x * 3 + 2] = 4e-8f; //NE
															//0.1

			// float gmax = 8.5e-7f;
			// float A = 4.f;																//anisotropy
			// float pars[6] = {0.1522, 0.0320, 0.2150, -34.2400, gmax, gmax / 10.f};		// lambda, alfa, beta, V0, Go, Gc

			float A = 3.f; //anisotropy
			float gmax = 3.e-8f;
			//cx43d
			// float pars[6] = {0.1522, 0.0320, 0.2150, -34.2400, gmax, gmax / 10.f};		// lambda, alfa, beta, V0, Go, Gc
			// float par[14] = {															//init, paskui keisim anis.
			// 				 pars[0], pars[1], pars[2], pars[3], -1, pars[4], pars[5],	//  % left side // lambda, alfa, beta, V0, Pol, Go, Gc
			// 				 pars[0], pars[1], pars[2], pars[3], -1, pars[4], pars[5]}; // % right side

			// Cx43/45 hetero
			// double gmax = 1.0e-9;                                                                        //.e-9;
			double par[14] = {0.0001, 0.0239, 0.0912, -19.0232, -1, 4 * 2 * gmax, 4 * 2 * 0.0773 * gmax, // left side
							  0.1115, 0.0179, 0.1051, -1.6781, -1, 2 * gmax, 2 * 0.0112 * gmax};		 // right side

			for (int i = 0; i < 3; i++)
			{ //W,NW,NE counter
				for (int ii = 0; ii < 4; ii++)
					ssb_gj_p_host[y * sizeX * 3 * 4 + x * 3 * 4 + i * 4 + ii] = 0.f;
				//sm4sm nereikia: ssb_gj_p_host[y * sizeX * 3 * 4 + x * 3 * 4 + i * 4 + 0] = 1.f; // initial - all open
				for (int ii = 0; ii < 7 * 2; ii++) //copy all parameters of all gates
				{
					ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + ii] = par[ii]; // visi par
																									   // if (i == 0 && ii >= 4 * 2 && ii < 4 * 4) { // W
																									   // 	ssb_gj_par_host[y * sizeX * 3 * 7 * 4 + x * 3 * 7 * 4 + i * 7 * 4 + ii] *= anis; // set anis
																									   // }
				}
			}
			int sel = 0;																	// select W
			ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 5] *= A;	//Go1 set anis
			ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 6] *= A;	//Gc1 set anis
			ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 12] *= A; //Go2 set anis
			ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 13] *= A; //Gc2 set anis

			// for (int sel = 0; sel < 3; sel++)
			// { //kliutis

			// 	// int r = 4;

			// 	// if (pow(x - 128, 2) + pow(y - 64, 2) <= pow(r + 1, 2))
			// 	// {																					   //sumazinto laidumo zona //				// if (!(x == 128 && y == 64 - 16 - 1)) //for debug: disable all gj except 1 cell
			// 	// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 5] *= 0.3f;  //Go1 set
			// 	// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 6] *= 0.3f;  //Gc1 set
			// 	// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 12] *= 0.3f; //Go2 set
			// 	// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 13] *= 0.3f; //Gc2 set
			// 	// }

			// 	if (
			// 		//pow(x - 128, 2) + pow(y - 64, 2) <= pow(r - 1, 2) // nulinio laidumo zona //nesuzadinamoj zonoj atjungiam vidurines last
			// 		// ( ( x >= 0 && x < 128) && (y >= 0 && y < 64) && abs((x-128)*0.02-(y-32)) < 1 )// vortex shedding bandymai
			// 		// vert.
			// 		(abs(x - 128 + 64) < 4 /*&& (y > 64-16 && y < 64+16)*/) // 1 sink-source mismatch bandymai
			// 		|| (abs(x - 128) < 16 && abs(y - 64) > 4)				// 2,3 sink-source mismatch bandymai
			// 		// || (abs(x - 128 - 64) < 2)								// 4 sink-source mismatch bandymai
			// 		// 	// || ( abs(x-128) < 4 && (y > 0 && y < 32 || y > 32+2 && y <= 64+60) ) //spyglys y: kord+r+spyglioIlgis
			// 		//horiz.
			// 		|| (abs(y - 64) - 8 == 8 && x < 128) // 1,2 sink-source mismatch bandymai
			// 											 // || (abs(y - 64) - 8 == 0 && x < 128) // 3,4 sink-source mismatch bandymai
			// 	)
			// 	{
			// 		float gjOpen = gmax * 0.003f; //1e-18f;
			// 		float gjClosed = gjOpen / 10.f;
			// 		ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 5] = gjOpen;	  //Go1 set anis
			// 		ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 6] = gjClosed;  //Gc1 set anis
			// 		ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 12] = gjOpen;	  //Go2 set anis
			// 		ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 13] = gjClosed; //Gc2 set anis
			// 	}
			// }

			//for (int i = 0; i < 1; i++)
			//ssb_dbg_host[y * sizeX * 1 + x * 1 + i] = 0;
		}
	for (int i = 0; i < i_frame_max; i++)
	{
		ssb_u_reg_host[i] = 0.f; //sin(float(i) / 100);
		ssb_gj_reg_host[0 * i_frame_max + i] = 0.f;
		ssb_gj_reg_host[1 * i_frame_max + i] = 0.f;
		ssb_dbg_host[i * 3 + 0] = 0.f;
		ssb_dbg_host[i * 3 + 1] = 0.f;
		ssb_dbg_host[i * 3 + 2] = 0.f;
	}
	// GLuint ssbo_u, ssbo_v, ssbo_w, ssbo_J_ion, ssbo_J_gj, ssbo_u_reg, ssbo_gj, ssbo_gj_reg, ssbo_gj_par, ssbo_gj_p, ssbo_dbg;
	glGenBuffers(1, &ssbo_u);
	glGenBuffers(1, &ssbo_v);
	glGenBuffers(1, &ssbo_w);
	glGenBuffers(1, &ssbo_J_ion);
	glGenBuffers(1, &ssbo_J_gj);
	glGenBuffers(1, &ssbo_u_reg);
	glGenBuffers(1, &ssbo_gj);
	glGenBuffers(1, &ssbo_gj_reg);
	glGenBuffers(1, &ssbo_gj_par);
	glGenBuffers(1, &ssbo_gj_p);
	glGenBuffers(1, &ssbo_dbg);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeX * sizeY * sizeof(float), ssb_u_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_u);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_v);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeX * sizeY * sizeof(float), ssb_v_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_v);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_w);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeX * sizeY * sizeof(float), ssb_w_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_w);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_J_ion);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeX * sizeY * sizeof(float), ssb_J_ion_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_J_ion);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_J_gj);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * sizeX * sizeY * sizeof(float), ssb_J_gj_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_J_gj);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u_reg);
	glBufferData(GL_SHADER_STORAGE_BUFFER, i_frame_max * sizeof(float), ssb_u_reg_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssbo_u_reg);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * sizeX * sizeY * sizeof(float), ssb_gj_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssbo_gj);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_reg);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * i_frame_max * sizeof(float), ssb_gj_reg_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, ssbo_gj_reg);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_par);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(float), ssb_gj_par_host, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, ssbo_gj_par);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_p);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 4 * sizeX * sizeY * sizeof(float), ssb_gj_p_host, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, ssbo_gj_p);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_dbg);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * i_frame_max * sizeof(float), ssb_dbg_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, ssbo_dbg);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); //unbind

	//uniform parameters
	struct Par
	{
		float dt = 0.02; //ms
	} par;
	glUseProgram(kernel1); //glUseProgram(shIDs.compID1);
	//GLint location = glGetUniformLocation(shIDs.compID1, "sizeXY"); printf("location %d \n", location);	if (location == -1) { printf("Could not locate uniform location in CS. "); }
	glUniform2ui(20, sizeX, sizeY);
	//location = glGetUniformLocation(shIDs.compID1, "dt_sim"); printf("location %d \n", location);	if (location == -1) { printf("Could not locate uniform location in CS. "); }
	glUniform1f(21, par.dt);
	const GLint location = glGetUniformLocation(kernel1, "gjModelEnabled");
	glUniform1ui(location, enableGjModel);
	glUseProgram(kernel2); //glUseProgram(shIDs.compID2);
	glUniform2ui(20, sizeX, sizeY);
	glUniform1f(21, par.dt);
	glUseProgram(vertFragShadersID); //glUseProgram(shIDs.frVeID);
	glUniform2ui(20, sizeX, sizeY);
	glUseProgram(line2DvertFragShadersID); //glUseProgram(shIDs.frVeID);
	glUniform2ui(20, sizeX, sizeY);

	float *vertices = new float[sizeX * sizeY * 4]; //*4 because xy screen, and xy cell nr.
	unsigned int *conn1 = new unsigned int[(sizeX - 1) * (sizeY - 1) * 3];
	unsigned int *conn2 = new unsigned int[(sizeX - 1) * (sizeY - 1) * 3];
	unsigned int *conn = new unsigned int[(sizeX - 1) * (sizeY - 1) * 6];

	for (unsigned int y = 0; y < sizeY; y++)
		for (unsigned int x = 0; x < sizeX; x++)
		{
			if ((y + 1) % 2)
				vertices[y * 4 * sizeX + x * 4] = (float)x / sizeX; //X screen
			else
				vertices[y * 4 * sizeX + x * 4] = (float)(x + 0.5) / sizeX; //X screen
			vertices[y * 4 * sizeX + x * 4 + 1] = (float)y / sizeY;			//Y screen
			((unsigned int *)(vertices))[y * 4 * sizeX + x * 4 + 2] = x;	//x cell nr. (int), konvertuojam i unsigned int, nes float ir int uzima tiek pat (32b)
			((unsigned int *)(vertices))[y * 4 * sizeX + x * 4 + 3] = y;	//y cell nr. (int)
		}

	for (unsigned int y = 0; y < sizeY - 1; y++)
		for (unsigned int x = 0; x < sizeX - 1; x++)
		{
			conn[y * 3 * 2 * (sizeX - 1) + x * 3 * 2] = y * sizeX + x;		   // size-1 nes paskutinis taskas neturi trikampio
			conn[y * 3 * 2 * (sizeX - 1) + x * 3 * 2 + 1] = y * sizeX + x + 1; //2-tra virsune
			if (y % 2 == 0)
			{
				conn[y * 3 * 2 * (sizeX - 1) + x * 3 * 2 + 2] = (y + 1) * sizeX + x; //3-cia virsune
				conn[y * 3 * 2 * (sizeX - 1) + x * 3 * 2 + 3] = y * sizeX + x + 1;
				conn[y * 3 * 2 * (sizeX - 1) + x * 3 * 2 + 4] = (y + 1) * sizeX + x + 1;
				conn[y * 3 * 2 * (sizeX - 1) + x * 3 * 2 + 5] = (y + 1) * sizeX + x;
			}
			else
			{
				conn[y * 3 * 2 * (sizeX - 1) + x * 3 * 2 + 2] = (y + 1) * sizeX + x + 1;
				conn[y * 3 * 2 * (sizeX - 1) + x * 3 * 2 + 3] = y * sizeX + x;
				conn[y * 3 * 2 * (sizeX - 1) + x * 3 * 2 + 4] = (y + 1) * sizeX + x + 1;
				conn[y * 3 * 2 * (sizeX - 1) + x * 3 * 2 + 5] = (y + 1) * sizeX + x;
			}
		}

	//*** prepare buffers for drawing hex map

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO); // svarbu, nes pagal ji piesiama (VBO tik storage)

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeX * sizeY * 4 * sizeof(float), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (sizeX - 1) * (sizeY - 1) * 2 * 3 * sizeof(unsigned int), conn, GL_STATIC_DRAW);

	// position on screen (xy) attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	// cell nr (xy) attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(int), (void *)(2 * sizeof(int))); // saugom int reiksmes float masyve, nes uzima tiek pat 32b
	glEnableVertexAttribArray(1);
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, VBO); // eksperimentai su vertex array modifikavimu per ssb

	//*** prepare buffers for drawing 2d line plot

	unsigned int VBO2, VBO3, VAO2, VAO3, EBO2;
	glGenVertexArrays(1, &VAO2);
	glGenVertexArrays(1, &VAO3);
	glGenBuffers(1, &VBO2);
	glGenBuffers(1, &VBO3);
	glGenBuffers(1, &EBO2);

	glBindVertexArray(VAO2);
	glBindBuffer(GL_ARRAY_BUFFER, VBO2);

	//unsigned int elem[] = { 1 };
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, 1 * sizeof(unsigned int), elem, GL_STATIC_DRAW);

	const int line_points_count = 2000; //width
	GLfloat line[line_points_count];
	for (int i = 0; i < line_points_count; i++)
	{
		line[i] = (i - line_points_count / 2) / float(line_points_count / 2);
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof line, line, GL_STATIC_DRAW);
	GLint attribute_coord1d = glGetAttribLocation(line2DvertFragShadersID, "coord1d");
	if (attribute_coord1d == -1)
	{
		printf("Could not locate \"attribute_coord1d\" in line2d shader. ");
	}
	glEnableVertexAttribArray(attribute_coord1d);
	glVertexAttribPointer(
		attribute_coord1d, // attribute
		1,				   // number of elements per vertex, here just x
		GL_FLOAT,		   // the type of each element
		GL_FALSE,		   // take our values as-is
		0,				   // no space between values
		0				   // use the vertex buffer object
	);

	glBindVertexArray(VAO3);
	glBindBuffer(GL_ARRAY_BUFFER, VBO3);

	GLfloat line2[line_points_count];
	for (int i = 0; i < line_points_count; i++)
	{
		line2[i] = (i - line_points_count / 2) / float(line_points_count / 2);
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof line2, line2, GL_STATIC_DRAW);
	attribute_coord1d = glGetAttribLocation(line2DvertFragShadersID, "coord1d");
	if (attribute_coord1d == -1)
	{
		printf("Could not locate \"attribute_coord1d\" in line2d shader. ");
	}
	glEnableVertexAttribArray(attribute_coord1d);
	glVertexAttribPointer(
		attribute_coord1d, // attribute
		1,				   // number of elements per vertex, here just x
		GL_FLOAT,		   // the type of each element
		GL_FALSE,		   // take our values as-is
		0,				   // no space between values
		0				   // use the vertex buffer object
	);

	glBindVertexArray(0);

	//***
	//some info print
	//int work_grp_cnt[3];
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
	//printf("max global (total) work group size x:%i y:%i z:%i\n",
	//	work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);
	//int work_grp_size[3];
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
	//glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
	//printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
	//	work_grp_size[0], work_grp_size[1], work_grp_size[2]);
	//int work_grp_inv;
	//glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
	//printf("max local work group invocations %i\n", work_grp_inv);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//width = sizeX*10; height = sizeY*10;
	//glViewport(0, 0, width, height);
	//glMatrixMode(GL_PROJECTION);
	//float aspect = (float)width / (float)height;
	//glOrtho(-aspect, aspect, -1, 1, -1, 1);

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();
	//glDrawElements(GL_TRIANGLES, (sizeX - 1) * (sizeY - 1) * 2 * 3, GL_UNSIGNED_INT, 0);

	int i = 0;
	int i_frame = 0;
	t_start = clock();
	do
	{
		if (!pause)
		{
			mtx.lock();
			glfwMakeContextCurrent(window);
			glClear(GL_COLOR_BUFFER_BIT);
			//Sleep(50);
			printf("\rframe %d, iter %d, simul %.2f ms", i_frame, i, i * .02f);
			for (int a = 0; a < itersInFrame; a++)
			{ // launch compute shaders!
				// glMemoryBarrier(GL_ALL_BARRIER_BITS);
				glUseProgram(kernel1); //glUseProgram(shIDs.compID1);
				glUniform1ui(22, i);   //butina, mc36ss, ---nebutina, debug
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				glDispatchCompute((GLuint)(sizeX + workGroupSize - 1) / workGroupSize, (GLuint)(sizeY + workGroupSize - 1) / workGroupSize, 1); // + workGroupSize - 1) tam kad butu "divide round up"
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				glUseProgram(kernel2); //glUseProgram(shIDs.compID2);
				glUniform1ui(22, i);
				// glMemoryBarrier(GL_ALL_BARRIER_BITS);
				glDispatchCompute((GLuint)(sizeX + workGroupSize - 1) / workGroupSize, (GLuint)(sizeY + workGroupSize - 1) / workGroupSize, 1);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);

				i++;
				// // for dbg
				// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_dbg);
				// glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 3*i_frame_max * sizeof(float), ssb_dbg_host);
				// glMemoryBarrier(GL_ALL_BARRIER_BITS);
				// // for (int a = 0; a < 4; a++) {
				// // 	printf("\n");
				// // 	for (int j = 0; j < 4; j++) {
				// // 		printf(" a%dj%d: %f", a, j, ssb_dbg_host[a*4+j]);
				// // 		/*if ((a + 1) % 36 == 0)
				// // 			printf("\nj=%d/n",j);*/
				// // 	}
				// // }
				// ofstream myfile;
				// myfile.open("trink.txt");
				// myfile << "t,vj,gj" << endl;
				// for (int i = 0; i < 10000; i++) {
				// 	myfile << ssb_dbg_host[i*3+0] << ',' << ssb_dbg_host[i*3+1] << ',' << ssb_dbg_host[i*3+2] << endl;
				// }
				// myfile.close();
				// cout << "Done.";

				// 			//if (i == 4)
				// exit(0);//return 0;
			}
			glUseProgram(vertFragShadersID); //;glUseProgram(shIDs.frVeID);
			//glPointSize(5.f);
			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //wireframe mode
			glBindVertexArray(VAO);
			glUniform1ui(26, 1); //whatToPlot: 1 u, 2 vj, 3 gj
			glDrawElements(GL_TRIANGLES, (sizeX - 1) * (sizeY - 1) * 2 * 3, GL_UNSIGNED_INT, 0);
			glUniform1ui(26, 2); //whatToPlot: 1 u, 2 vj, 3 gj
			glDrawElements(GL_TRIANGLES, (sizeX - 1) * (sizeY - 1) * 2 * 3, GL_UNSIGNED_INT, 0);
			glUniform1ui(26, 3); //whatToPlot: 1 u, 2 vj, 3 gj
			glDrawElements(GL_TRIANGLES, (sizeX - 1) * (sizeY - 1) * 2 * 3, GL_UNSIGNED_INT, 0);

			glUseProgram(line2DvertFragShadersID);
			glUniform1ui(25, i_frame);
			glUniform1ui(26, 1); //whatToPlot: 1 u, 2 gj
			// glMemoryBarrier(GL_ALL_BARRIER_BITS);
			glBindVertexArray(VAO2);
			glDrawArrays(GL_LINE_STRIP, 0, line_points_count);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			glUniform1ui(26, 2); //whatToPlot: 1 u, 2 gj
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			glDrawArrays(GL_LINE_STRIP, 0, line_points_count);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			// // Swap buffers
			glfwSwapBuffers(window);
			glfwPollEvents();
			if (false)
				if (lbutton_down)
				{
					// do your drag here
					double xpos, ypos;
					glfwGetCursorPos(window, &xpos, &ypos);
					//std::cout << "cursor: " << xpos << " | " << ypos << std::endl;
					//printf("(x%f;y%f)\n", xpos, ypos);
					/*int cX = sizeX*( (xpos/width*2 - 1.f)-.5f );*/
					//float x0 = (0 - .5) / 2.f; // konvertuojam poslinki (pagal vertex shader paslinkimus) /2.f nes opengl -1..+2, t.y. intervalas 2
					//int cX = sizeX * (xpos - width * (.5f/2.f)) / (width * .5f) ;
					float s = .7f;
					float x0 = 1. - 1.f;
					float y0 = 1. - .21f;
					int cX = sizeX * ((xpos - width * x0 / 2.f) / (width * .5f * s)); // *.5 nes 0..1 sudaro 0.5 viso range (0..2)
					int cY = sizeY * ((height - ypos) - height * y0 / 2.f) / (height * .5f * s);
					//float y0 = (0-.8) * 1.2f * height;
					//float y1 = (1.f-.8) * 1.2f * height;
					//int cY = sizeY * ( (height - ypos) - y0 ) / (y1-y0) ;//* (1.f-.8f)/2.f / 1.2f) / (height * 0.8f*2.f/2.f * 1.2f);
					//printf("\n(cxy %d : %d)\n", cX, cY);
					if (cX > 0 && cX < sizeX && cY > 0 && cY < sizeY)
					{
						glUseProgram(kernel2);
						GLint location = glGetUniformLocation(kernel2, "mouse_pressed");
						if (location == -1)
						{
							printf("Could not locate uniform location: mouse_pressed");
						}
						glUniform1ui(location, true);
						location = glGetUniformLocation(kernel2, "mouse_coords");
						if (location == -1)
						{
							printf("Could not locate uniform location: mouse_coords");
						}
						//unsigned int cX = 16, cY = 16;
						// TODO:
						glUniform2ui(location, (unsigned int)cX, (unsigned int)cY);
						glMemoryBarrier(GL_ALL_BARRIER_BITS);
					}
				}
				else
				{
					glUseProgram(kernel2);
					const GLint location = glGetUniformLocation(kernel2, "mouse_pressed");
					glUniform1ui(location, false);
					glMemoryBarrier(GL_ALL_BARRIER_BITS);
				}

			glUseProgram(kernel1);
			const GLint location = glGetUniformLocation(kernel1, "gjModelEnabled");
			glUniform1ui(location, enableGjModel);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);

			if (increaseGj)
			{
				increaseGj = false;
				for (int x = 0; x < sizeX; x++)
					for (int y = 0; y < sizeY; y++)
					{
						for (int i = 0; i < 3; i++)
						{																					 //W,NE,NE counter // bendras laidumas
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 5] *= 1.1f;	 //Go1 set
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 6] *= 1.1f;	 //Gc1 set
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 12] *= 1.1f; //Go2 set
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 13] *= 1.1f; //Gc2 set
						}
					}
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_par);
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(float), ssb_gj_par_host, GL_DYNAMIC_COPY);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				printf(" Go,Gc (NW) = ");
				int i = 1; //for (int i = 0; i < 3; i++) // W, NW, NE
				{
					int x = 2, y = 2; //spauzdinimui ekrane, imam tarkim 2;2 last
					float go[4] = {
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 5],	 //Go1
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 6],	 //Gc1
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 12], //Go2
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 13]	 //Gc2
					};
					printf("%.2e, %.2e, %.2e, %.2e, sel=%d; ", go[0], go[1], go[2], go[3], i);
				}
				printf("\n");
			}

			if (decreaseGj)
			{
				decreaseGj = false;
				for (int x = 0; x < sizeX; x++)
					for (int y = 0; y < sizeY; y++)
					{
						for (int i = 0; i < 3; i++)															 // bendras laidumas
						{																					 //W,NE,NE counter
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 5] /= 1.1f;	 //Go1 set
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 6] /= 1.1f;	 //Gc1 set
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 12] /= 1.1f; //Go2 set
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 13] /= 1.1f; //Gc2 set
						}
					}
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_par);
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(float), ssb_gj_par_host, GL_DYNAMIC_COPY);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				printf(" Go,Gc (NW) = ");
				int i = 1; //for (int i = 0; i < 3; i++) // W, NW, NE
				{
					int x = 2, y = 2; //spauzdinimui ekrane, imam tarkim 2;2 last
					float go[4] = {
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 5],	 //Go1
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 6],	 //Gc1
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 12], //Go2
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 13]	 //Gc2
					};
					printf("%.2e, %.2e, %.2e, %.2e, sel=%d; ", go[0], go[1], go[2], go[3], i);
				}
				printf("\n");
			}

			if (increaseGjZone)
			{
				increaseGjZone = false;
				for (int x = 0; x < sizeX; x++)
					for (int y = 0; y < sizeY; y++)
					{
						for (int sel = 0; sel < 3; sel++) //kliutis
						{
							int r = 4;
							if ((x >= 10 && x < 128) && (y >= 0 && y < 64) && abs(x * 0.02 - (y - 32)) < 1)
							//(pow(x - 128, 2) + pow(y - 64, 2) <= pow(r - 1, 2))
							{
								ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 5] *= 1.1f;  //Go1 set
								ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 6] *= 1.1f;  //Gc1 set
								ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 12] *= 1.1f; //Go2 set
								ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 13] *= 1.1f; //Gc2 set
							}
						}
					}
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_par);
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(float), ssb_gj_par_host, GL_DYNAMIC_COPY);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				printf(" Go,Gc (NW) = ");
				int i = 1; //for (int i = 0; i < 3; i++) // W, NW, NE
				{
					int x = 128, y = 64; //spauzdinimui ekrane, imam tarkim 2;2 last
					float go[4] = {
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 5],	 //Go1
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 6],	 //Gc1
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 12], //Go2
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 13]	 //Gc2
					};
					printf("%.2e, %.2e, %.2e, %.2e, sel=%d; ", go[0], go[1], go[2], go[3], i);
				}
				printf("\n");
			}

			if (decreaseGjZone)
			{
				decreaseGjZone = false;
				for (int x = 0; x < sizeX; x++)
					for (int y = 0; y < sizeY; y++)
					{
						for (int sel = 0; sel < 3; sel++) //kliutis
						{
							int r = 4;
							if ((x >= 10 && x < 128) && (y >= 0 && y < 64) && abs(x * 0.02 - (y - 32)) < 1)
							//(pow(x - 128, 2) + pow(y - 64, 2) <= pow(r - 1, 2))
							{
								ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 5] /= 1.1f;  //Go1 set
								ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 6] /= 1.1f;  //Gc1 set
								ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 12] /= 1.1f; //Go2 set
								ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 13] /= 1.1f; //Gc2 set
							}
						}
					}
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_par);
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(float), ssb_gj_par_host, GL_DYNAMIC_COPY);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				printf(" Go,Gc (NW) = ");
				int i = 1; //for (int i = 0; i < 3; i++) // W, NW, NE
				{
					int x = 128, y = 64; //spauzdinimui ekrane, imam tarkim 2;2 last
					float go[4] = {
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 5],	 //Go1
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 6],	 //Gc1
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 12], //Go2
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 13]	 //Gc2
					};
					printf("%.2e, %.2e, %.2e, %.2e, sel=%d; ", go[0], go[1], go[2], go[3], i);
				}
				printf("\n");
			}

			if (increaseSensitivity)
			{
				increaseSensitivity = false;
				float v0 = 0;
				for (int x = 0; x < sizeX; x++)
					for (int y = 0; y < sizeY; y++)
					{
						for (int i = 0; i < 3; i++) //W,NE,NE counter
						{
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 0 * 7 + 3] /= 1.1f; //V0_1
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 1 * 7 + 3] /= 1.1f; //V0_2
							v0 = ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 0 * 7 + 3];
						}
					}
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_par);
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(float), ssb_gj_par_host, GL_DYNAMIC_COPY);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				printf(" v0 = ");
				printf("%f, ", v0);
				printf("\n");
			}

			if (decreaseSensitivity)
			{
				decreaseSensitivity = false;
				float v0 = 0;
				for (int x = 0; x < sizeX; x++)
					for (int y = 0; y < sizeY; y++)
					{
						for (int i = 0; i < 3; i++) //W,NE,NE counter
						{
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 0 * 7 + 3] *= 1.1; //V0_1
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 1 * 7 + 3] *= 1.1; //V0_2
							v0 = ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 0 * 7 + 3];
						}
					}
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_par);
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(float), ssb_gj_par_host, GL_DYNAMIC_COPY);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				printf(" v0 = ");
				printf("%f, ", v0);
				printf("\n");
			}

			if (increaseAnis)
			{
				increaseAnis = false;
				// for (int x = 0; x < sizeX; x++)
				// 	for (int y = 0; y < sizeY; y++)
				// 	{
				// 		for (int i = 0; i < 3; i++)
				// 		{									   //W,NE,NE counter
				// 		ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 0*7+5];	 //Go1
				// 		ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 0*7+6];	 //Gc1
				// 		ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 1*7+5]; //Go2
				// 		ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 1*7+6]; //Gc2
				// 		}
				// 	}
				// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_par);
				// glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 4 * 7 * sizeX * sizeY * sizeof(float), ssb_gj_par_host, GL_DYNAMIC_COPY);
				// glMemoryBarrier(GL_ALL_BARRIER_BITS);
				// float goW = ssb_gj_par_host[0 * sizeX * 3 * 7 * 4 + 0 * 3 * 7 * 4 + 0 * 7 * 4 + (2 * 4 + 0)];
				// float goNW = ssb_gj_par_host[0 * sizeX * 3 * 7 * 4 + 0 * 3 * 7 * 4 + 1 * 7 * 4 + (2 * 4 + 0)];
				// printf(" Anis = %f\n", goW / goNW);
				printf("not implemented");
			}

			if (decreaseAnis)
			{
				decreaseAnis = false;
				// for (int x = 0; x < sizeX; x++)
				// 	for (int y = 0; y < sizeY; y++)
				// 	{
				// 		for (int i = 0; i < 3; i++)
				// 		{									   //W,NE,NE counter
				// 			for (int ii = 0; ii < 7 * 4; ii++) //copy all parameters of all gates
				// 			{
				// 				if (i == 0 && ii >= 4 * 2 && ii < 4 * 4)
				// 				{ // išrenkam tik Go
				// 					if (i == 0)
				// 						ssb_gj_par_host[y * sizeX * 3 * 7 * 4 + x * 3 * 7 * 4 + i * 7 * 4 + ii] /= 1.1f;
				// 				}
				// 			}
				// 		}
				// 	}
				// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_par);
				// glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 4 * 7 * sizeX * sizeY * sizeof(float), ssb_gj_par_host, GL_DYNAMIC_COPY);
				// glMemoryBarrier(GL_ALL_BARRIER_BITS);
				// float goW = ssb_gj_par_host[0 * sizeX * 3 * 7 * 4 + 0 * 3 * 7 * 4 + 0 * 7 * 4 + (2 * 4 + 0)];
				// float goNW = ssb_gj_par_host[0 * sizeX * 3 * 7 * 4 + 0 * 3 * 7 * 4 + 1 * 7 * 4 + (2 * 4 + 0)];
				// printf(" Anis = %f\n", goW / goNW);
				printf("not implemented");
			}
			glfwMakeContextCurrent(NULL);
			mtx.unlock();
			if (i_frame < i_frame_max)
				i_frame++;
			else
				//i_frame = 0;
				break;
			std::this_thread::sleep_for(std::chrono::microseconds(100));
		}

		GLenum err = glGetError();
		if (err)
		{
			// TODO error callback
			//	cout << "\nglGetError reported Error:" << (int)err << endl;
			// cout << "\nError string: " << glewGetErrorString(err) << endl;
		}

		while (!queueCmd.empty()) // grpc komandu apdorojimas
		{
			int cmd = queueCmd.front();
			printf("\nQueue=%d\n", cmd);
			queueCmd.pop();
			switch (cmd)
			{
			case 1: //pause
				/* code */
				pause = !pause;
				break;
			case 2: //getVm
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u);
				glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeX * sizeY * sizeof(float), ssb_u_host);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				queueReply.push(ssb_u_host);
				break;

			default:
				break;
			}
		}
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0);

	t_end = clock();

	glfwMakeContextCurrent(window);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 3 * sizeX * sizeY * sizeof(float), ssb_gj_host);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_reg);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 2 * i_frame_max * sizeof(float), ssb_gj_reg_host);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//export data to file
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_dbg);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 3 * i_frame_max * sizeof(float), ssb_dbg_host);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	std::ofstream myfile;
	myfile.open("reg.csv");
	myfile << "t,v1,v2,v_end,gj_reg1,gj_reg2\n";
	for (int i = 0; i < i_frame; i++)
		myfile << i * itersInFrame * par.dt << ","
			   << ssb_dbg_host[i * 3 + 0] << ","
			   << ssb_dbg_host[i * 3 + 1] << ","
			   << ssb_dbg_host[i * 3 + 2] << ","
			   << ssb_gj_reg_host[0 * i_frame_max + i] << ","
			   << ssb_gj_reg_host[1 * i_frame_max + i]
			   << std::endl;
	myfile.close();
	printf("\n\nResults file 'reg.csv' wrote.");

	// Cleanup VBO
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &ssbo_u);
	glDeleteBuffers(1, &ssbo_v);
	glDeleteBuffers(1, &ssbo_w);
	glDeleteBuffers(1, &ssbo_J_ion);
	glDeleteBuffers(1, &ssbo_J_gj);
	glDeleteBuffers(1, &ssbo_u_reg);
	delete[] ssb_u_host, ssb_v_host, ssb_w_host, ssb_J_ion_host, ssb_gj_host, ssb_u_reg_host;
	//glDeleteBuffers(1, &vertexbuffer);
	//glDeleteVertexArrays(1, &VertexArrayID);
	//glDeleteProgram(shIDs.frVeID);
	//glDeleteProgram(shIDs.compID1);
	//glDeleteProgram(shIDs.compID2);
	glDeleteProgram(kernel1);
	glDeleteProgram(kernel2);
	glDeleteProgram(vertFragShadersID);
	glDeleteProgram(line2DvertFragShadersID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	double execTime = (double)(t_end - t_start) / CLOCKS_PER_SEC;
	printf("\nTime required for execution: %f sec., FPS averange: %f\n", execTime,
		   i_frame / execTime);
	//system("PAUSE");

	(*serverPtr)->Shutdown();
	serverThread.join();
	return 0;
}

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (GLFW_PRESS == action)
			lbutton_down = true;
		else if (GLFW_RELEASE == action)
			lbutton_down = false;
	}

	/*
		if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {

		}*/
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_G:
			enableGjModel = !enableGjModel;
			printf(", enableGjModel=%d\n", enableGjModel);
			break;
		case GLFW_KEY_KP_7: // laidumas
			increaseGj = true;
			printf(", increaseGj, ");
			break;
		case GLFW_KEY_KP_4:
			decreaseGj = true;
			printf(", decreaseGj, ");
			break;
		case GLFW_KEY_KP_1: // laidumas zona
			increaseGjZone = true;
			printf(", increaseGjZone, ");
			break;
		case GLFW_KEY_KP_0:
			decreaseGjZone = true;
			printf(", decreaseGjZone, ");
			break;
		case GLFW_KEY_KP_8: // jautrumas V0
			increaseSensitivity = true;
			printf(", increaseSensitivity, ");
			break;
		case GLFW_KEY_KP_5:
			decreaseSensitivity = true;
			printf(", decreaseSensitivity, ");
			break;
		case GLFW_KEY_KP_9: // aniz
			increaseAnis = true;
			printf(", increaseAnis, ");
			break;
		case GLFW_KEY_KP_6:
			decreaseAnis = true;
			printf(", decreaseAnis, ");
			break;
		default:
			break;
		}
	}
}

void APIENTRY glDebugOutput(GLenum source,
							GLenum type,
							unsigned int id,
							GLenum severity,
							GLsizei length,
							const char *message,
							const void *userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
		return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:
		std::cout << "Source: API";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		std::cout << "Source: Window System";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		std::cout << "Source: Shader Compiler";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		std::cout << "Source: Third Party";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		std::cout << "Source: Application";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		std::cout << "Source: Other";
		break;
	}
	std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
		std::cout << "Type: Error";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		std::cout << "Type: Deprecated Behaviour";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		std::cout << "Type: Undefined Behaviour";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		std::cout << "Type: Portability";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		std::cout << "Type: Performance";
		break;
	case GL_DEBUG_TYPE_MARKER:
		std::cout << "Type: Marker";
		break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		std::cout << "Type: Push Group";
		break;
	case GL_DEBUG_TYPE_POP_GROUP:
		std::cout << "Type: Pop Group";
		break;
	case GL_DEBUG_TYPE_OTHER:
		std::cout << "Type: Other";
		break;
	}
	std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:
		std::cout << "Severity: high";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		std::cout << "Severity: medium";
		break;
	case GL_DEBUG_SEVERITY_LOW:
		std::cout << "Severity: low";
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		std::cout << "Severity: notification";
		break;
	}
	std::cout << std::endl;
	std::cout << std::endl;
}

//*******************
//************GRPC***********
//**************

// Logic and data behind the server's behavior.
class FentonControlServiceImpl final : public FentonControl::Service
{
	//Status StopSim(ServerContext *context, const ::google::protobuf::Empty *request, ::google::protobuf::Empty *response) override //veikia
	Status StopSim(ServerContext *context, const helloworld::Empty *request, helloworld::Empty *response) override
	{
		printf("StopSim\n");
		queueCmd.push(1); // pause
		return Status::OK;
	}
	Status getVm(ServerContext *context, const ::google::protobuf::Empty *request,
				 VmReply *reply) override
	{
		cout << "\nLocke requested\n";
		mtx.lock();
		cout << "\nLocked\n";
		glfwMakeContextCurrent(window);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeX * sizeY * sizeof(float), ssb_u_host);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		// glClear(GL_COLOR_BUFFER_BIT);
		// std::this_thread::sleep_for(std::chrono::seconds(3));
		glfwMakeContextCurrent(NULL);
		mtx.unlock();
		cout << "\nUnlocked\n";
		float *Vm = ssb_u_host;

		// float ar[2][2] = {{1, 2}, {3, 4}};
		// reply->set_vm();
		reply->set_x(sizeX);
		reply->set_y(sizeY);
		// for (int x = 0; x < sizeX; x++)
		// 	for (int y = 0; y < sizeY; y++)
		// 		reply->add_vm(ar[y][x]);
		// cout << "Vm size = " << reply->vm_size() << endl;
		for (int y = 0; y < sizeY; y++)
			for (int x = 0; x < sizeX; x++)
				reply->add_vm(Vm[y * sizeX + x]);
		// break;

		// default:
		// 	break;
		// }

		return Status::OK;
	}
	Status SayHello2(ServerContext *context, const HelloRequest *request,
					 HelloReply2 *reply) override
	{
		// std::string prefix("Hello ");
		reply->set_messageint32(12);
		reply->set_messagefloat(13.3);
		return Status::OK;
	}
};

void RunServer(std::unique_ptr<Server> *serverPtr) // serverPtr - to control server (shitdown) from another thread
{
	std::string server_address("0.0.0.0:8001");
	FentonControlServiceImpl service;

	ServerBuilder builder;
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	// Register "service" as the instance through which we'll communicate with
	// clients. In this case it corresponds to an *synchronous* service.
	builder.RegisterService(&service);
	// Finally assemble the server.
	std::unique_ptr<Server> server(builder.BuildAndStart());
	// std::unique_ptr<Server> *server = new std::unique_ptr<Server>(builder.BuildAndStart());
	// serverPtr = server; //new Server(builder.BuildAndStart());
	serverPtr = &server;
	std::cout << "Server listening on " << server_address << std::endl;

	// Wait for the server to shutdown. Note that some other thread must be
	// responsible for shutting down the server for this call to ever return.
	// (*server)->Wait();
	server->Wait();
}