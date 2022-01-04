#include <filesystem> // to delete previous zarr files

#include "nlohmann/json.hpp"
#include "xtensor/xarray.hpp"

// #include "xtensor-zarr/xzarr_hierarchy.hpp"
// #include "xtensor-zarr/xzarr_file_system_store.hpp"

// #include <xtensor/xarray.hpp>
// #include <xtensor/xio.hpp>
// #include <xtensor/xview.hpp>

// factory functions to create files, groups and datasets
#include "z5/factory.hxx"
// handles for z5 filesystem objects
#include "z5/filesystem/handle.hxx"
// io for xtensor multi-arrays
#include "z5/multiarray/xtensor_access.hxx"
// attribute functionality
#include "z5/attributes.hxx"

#include <memory>
#include <string>
#include <random>
#include <iterator>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/empty.pb.h>
#include "fentonControl.grpc.pb.h"

#include "npy.hpp"

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
// using namespace std;

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
queue<float *> queueReply; // kai float
// queue<double *> queueReply; // kai double

const size_t sizeX = 50; // lasteliu turi buti lyginis skaicius, kad veiktu periodic boudaries
const size_t sizeY = 2;
const size_t workGroupSizeX = 32; //32
const size_t workGroupSizeY = 32; //32
const size_t itersInFrame = 1;  // dt_reg = dt_sim * itersInFrame
// **** Simulacijos trukme
// const float t_max = 160000.f; //ms
const float t_max = 400000.f; //ms
const size_t frame_chunk_size = 100; // kiek i_frame itaraciju vienam chunk
// Open a window and create its OpenGL context
int scale = 600;
int width = scale * sizeX / sizeY; //*2 kai 2 2d grafikai
int height = scale;
//uniform parameters
struct Par {
	float  dt = 0.02; //ms //kai float
} par;



//Shader storege buffer. For compute shader
float *ssb_u_host = new float[sizeX * sizeY];
float *ssb_v_host = new float[sizeX * sizeY];
float *ssb_w_host = new float[sizeX * sizeY];
float *ssb_J_ion_host = new float[sizeX * sizeY];
float *ssb_gj_host = new float[sizeX * sizeY * 3]; // *3: W,NW,NE
float *ssb_gj_par_host = new float[sizeX * sizeY * 3 * 7 * 2]; // *3: W,NW,NE; *7: parameters count; *2:gates count
float *ssb_gj_p_host = new float[sizeX * sizeY * 3 * 4];	   // markov state matices for each gj
float *ssb_J_gj_host = new float[sizeX * sizeY * 3]; // *3: W,NW,NE
// reg:
float *ssb_u_reg_host = new float[frame_chunk_size*sizeX * sizeY]; // kolkas neisvedama?
float *ssb_gj_reg_host = new float[frame_chunk_size * sizeX * sizeY * 3];
const int dbg_size = 10;
float *ssb_dbg_host = new float[frame_chunk_size * dbg_size]; // 10 laisvu vietu

GLuint ssbo_u, ssbo_v, ssbo_w, ssbo_J_ion, ssbo_J_gj, ssbo_u_reg, ssbo_gj, ssbo_gj_reg, ssbo_gj_par, ssbo_gj_p, ssbo_dbg;

std::mt19937 rng;

int main(void)
{
	rng.seed(123); // for initializing heterotypic GJs distribution in tissue
	std::uniform_int_distribution<uint32_t> uint_dist100(0,100);

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
	GLuint kernel1 = LoadComputeShader("C:\\OpenGL\\fentonGjOpenGL\\compShader1.comp", workGroupSizeX, workGroupSizeY);
	GLuint kernel2 = LoadComputeShader("C:/OpenGL/fentonGjOpenGL/compShader2.comp", workGroupSizeX, workGroupSizeY);
	GLuint vertFragShadersID = LoadVertexFragmentShaders("C:/OpenGL/fentonGjOpenGL/SimpleVertexShader.vert", "C:/OpenGL/fentonGjOpenGL/SimpleFragmentShader.frag");
	GLuint line2DvertFragShadersID = LoadVertexFragmentShaders("C:/OpenGL/fentonGjOpenGL/linePlot.vert", "C:/OpenGL/fentonGjOpenGL/linePlot.frag");

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

			// float g_both = 50e-9f;
			// float g_both = 50e-9f;
			// float g_both = 8e-9f;
			float A = 2.f; //anisotropy
			
			float gmax43 = 300e-9f; //600 300
			float gmax4543 = 46e-9f;//64 46
			// float gmax43 = 300e-9f; // susidaro prie 75-120-240 pulso
			// float gmax4543 = 45e-9f;
			//cx43
			float pars43[6] = {0.1522, 0.0320, 0.2150, -34.2400, gmax43, gmax43 / 10.f};			  // lambda, alfa, beta, V0, Go, Gc
			float par43[14] = {																		  //init, paskui keisim anis.
							   pars43[0], pars43[1], pars43[2], pars43[3], -1, pars43[4], pars43[5],  //  % left side // lambda, alfa, beta, V0, Pol, Go, Gc
							   pars43[0], pars43[1], pars43[2], pars43[3], -1, pars43[4], pars43[5]}; // % right side

//  % Vj gating parameters of Cx43EGFP/Cx45 gap junction
// %        lamda     A_alfa    A_beta     V_0     P_g      G_o             G_c
// par =  [ 0.0656    0.0346    0.1922  -30.6046   -1   2.396*2*gmax  2.396*2*0.0773*gmax;    % Cx43 side
//          0.1005    0.0390    0.0694  -14.4894   -1     2*gmax            2*0.0157*gmax];   % Cx45 side   
// limit = 4.5508;     % threshold transition rate 

			// Cx43/45 hetero, mindaugo 2021-06-28
			// double gmax = 1.0e-9;                                                                        //.e-9;
			// gmax4543 *= 1e-11;

			// // seni
			// float gmax4543 = g_both*0.4;
			// double par4543[14] = {0.0656, 0.0346, 0.1922, 30.6046, 1,  2.396 * 2 * gmax4543,  2.396 * 2 * 0.0773 * gmax4543, // left side
			// 					  0.1005, 0.0390, 0.0694, 14.4894, 1, 2 * gmax4543, 2 * 0.0157 * gmax4543};		 // right side

			// MS 2021-09-15
			// float gmax4543 = 1.0f;
			// float gmax4543 = g_both*3.15;
			// double par4543[14] = {0.0462, 0.0264 , 0.1525, -7.8248, -1,  gmax4543,  0.0033 * gmax4543, // left side
			// 					  0.0567, 0.0159, 0.3085, -15.0997, -1, gmax4543 * 0.158, 0.0004 * gmax4543 * 0.158};		 // right side, 0.158 pagal 45 : 43 open busenu laidumu santyki
			
// {"lambda1":0.7083354891481388,"A_alpha1":0.35398071557426974,"A_beta1":0.1446391702724877,"V01":-75.12289637286416,"polarity1":-1,"Go1":1,"Gc1":0.024245815743767676,
// "lambda2":0.04123104898401486,"A_alpha2":0.03370471203601321,"A_beta2":0.133229953423052,"V02":-15.266576202701913,"polarity2":-1,"Go2":0.158,"Gc2":0.010556548377678457,"limit":2.1815270775900486}
			// // Cx43/45 hetero, KM 2021-10-06 'par + card(25proc)+70_hyst+-80_step -V0 v2.json'
			// // float gmax4543 = 1.0f;
			// float gmax4543 = g_both*0.5*1.47*3.72*0.6;
			// double par4543[14] = {0.7083354891481388, 0.35398071557426974 , 0.1446391702724877, -75.12289637286416, -1,  gmax4543,  gmax4543*0.024245815743767676, // left side
			// 					  0.04123104898401486, 0.03370471203601321, 0.133229953423052, -15.266576202701913, -1, gmax4543*0.158, gmax4543 * 0.010556548377678457};	

			// MS 2021-11-04
			// float gmax4543 = g_both*0.8;

			double par4543[14] = {0.0257, 0.0218, 0.0899, -37.5, -1, gmax4543*0.8876, gmax4543*0.07,
				   0.0525, 0.039, 0.0993, -5.22, -1,  gmax4543*0.816,  gmax4543*0.0125};	
			double limit = 4.83; // <--- TODO: kolkas limit reikia rankiniu budu ivesti i compShader1
			// // KM 2021-10-29
			// float gmax4543 = g_both*0.5;
			// double par4543[14] = {0.0257, 0.0218, 0.0899, -37.5, -1, gmax4543*0.8876, gmax4543*0.07,
			// 	   0.0525, 0.039, 0.0993, -5.22, -1,  gmax4543*0.816,  gmax4543*0.0125};	
			// double limit = 4.83; // <--- TODO: kolkas limit reikia rankiniu budu ivesti i compShader1
			// // MS 2021-10-16
			// float par_mind[13] = {0.0943, 0.1054, 0.0416, -1.7565, 0.0377, 0.0403, 0.0336, 0.2035, -18.3999, 0.0262, 31.4403, 98.1429, 0.1135};
			// float gmax4543 = g_both*0.015;
			// double par4543[14] = {par_mind[5], par_mind[6], par_mind[7], par_mind[8], -1, gmax4543*par_mind[11], gmax4543*par_mind[11]*par_mind[9],
			// 	   par_mind[0], par_mind[1], par_mind[2], par_mind[3], -1,  gmax4543*par_mind[10],  gmax4543*par_mind[10]*par_mind[4]};	
			// double limit = par_mind[12]; // <--- TODO: kolkas limit reikia rankiniu budu ivesti i compShader1
			
			// // Cx43/45 hetero bkp
			// // double gmax = 1.0e-9;                                                                        //.e-9;
			// float gmax4543 = g_both*0.6;
			// // gmax4543 *= 1e-11;
			// double par4543[14] = {0.0001, 0.0239, 0.0912, -19.0232, -1, 4 * 2 * gmax4543, 4 * 2 * 0.0773 * gmax4543, // left side
			// 					  0.1115, 0.0179, 0.1051, -1.6781, -1, 2 * gmax4543, 2 * 0.0112 * gmax4543};		 // right side

			// nustatom PJ param(g dar bus keiciamas)
			for (int i = 0; i < 3; i++) //W,NW,NE counter
			{
				for (int ii = 0; ii < 4; ii++)
					ssb_gj_p_host[y * sizeX * 3 * 4 + x * 3 * 4 + i * 4 + ii] = 0.f;
				for (int ii = 0; ii < 7 * 2; ii++)														 //copy all parameters of all gates
					ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + ii] = par43[ii]; // visi par
			}

			// pradzioj laikom kad visos PJ nelaidzios
			for (int sel = 0; sel < 3; sel++)
			{
				float gjOpen = 1.e-30f;
				float gjClosed = 1.e-30f;
				ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 5] = gjOpen;	  //Go1 set anis
				ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 6] = gjClosed;  //Gc1 set anis
				ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 12] = gjOpen;	  //Go2 set anis
				ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 13] = gjClosed; //Gc2 set anis
			}

			// pridedam laidzia zona
			// if (x < y && (-x + sizeY) > y  ||  x < y && (-x + sizeY) > y) // zonos matmenys
			// triangles lines & middle channel box
			// float k = 1.f;
			float k = 1.f;
			int middleBoxHeight = 6;////sizeY / 2;
			int topY = sizeY / 2 + middleBoxHeight/2;
			int bottomY = sizeY / 2 - middleBoxHeight/2;
			// if ( // trikampe zona
			// 	x * k < y					// right bottom
			// 		&& (-x * k + sizeY) > y // right top
			// 	// ||
			// 	// (-x + sizeY) * k < y			   // left bottom
			// 	// 	&& (x - sizeX) * k + sizeY > y // left top
			// 	||
			// 	y < topY && y > bottomY // middle channel box
			// )
			{
				float storis = 4.f;
				for (int i = 0; i < 3; i++)									//W,NW,NE counter
				{
					// const float cx_change_rate = 0.9;
					// bool is4345 = uint_dist100(rng) <= 60 * ( (float)x/sizeX*cx_change_rate );
					bool isNonCond = uint_dist100(rng) >= 60; // cx43 ar cx4345 heterotyp. tikymybe
					bool is4345 = uint_dist100(rng) >= 40; // cx43 ar cx4345 heterotyp. tikymybe
					bool orientation = uint_dist100(rng) >= 50; // orientacija erdveje issibarsciusi vienodomis tikimybemis
					for (int ii = 0; ii < 7 * 2; ii++)						//copy all parameters of all gates
					{
						// if (x > 32 - storis / 2.f && x < 32 + storis / 2.f) //heterotypic box, Cx43-45 zona		// {// "brick wall", hetero-line: W, NW - cx4345 p(+); NE - cx4345 p(-)
						// if (x > 32 - storis / 2.f && x < 32 + storis / 2.f && uint_dist10(rng) > 7) //heterotypic box, Cx43-45 zona		// {// "brick wall", hetero-line: W, NW - cx4345 p(+); NE - cx4345 p(-)
						// if (x > 32 - storis / 2.f && is4345) // su random distribution
						
						// if (x >= sizeX-1) { // add non conductive line

						// }
						// else

						// if (x > 32 - storis / 2.f && x < 32 + storis / 2.f) //heterotypic box, Cx43-45 zona		// {// "brick wall", hetero-line: W, NW - cx4345 p(+); NE - cx4345 p(-)
						// if (x > 30 ) //heterotypic box, Cx43-45 zona		
						if (x > 10 && (x % 20 > 0) && (x % 20 <= 2) ) //kas 20 po 2 last, Cx43-45 zona		straipsniui
						// if (x > 10 && (x % 20 > 0) && (x % 20 <= 10) ) //cv bandymai, Cx43-45 zona		
						// if (is4345 && i != 1) //reentry tyrimams Cx43-45 zona		// {// "brick wall", hetero-line: W, NW - cx4345 p(+); NE - cx4345 p(-)
						// if (x >= 70 && x <= 72 && y >= 75-50 && y < 75+50)
							ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + ii] = par4543[ii];
						// if (i == 2) //NE,
						// ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + ii] = par4543[ii];;
						// }
						else
						// if (!isNonCond) // Cx43 zona
						   {
							// if (isNonCond && ( x >= 15 && (x % 10 == 3) ) ) { // cx43 zonoje pridedam kliuciu, kad sumazinti sroves nusiurbima
								// if (ii == 5 || ii == 6 || ii == 12 || ii == 13) //go1,2 gc1,2 = labai mazas
									// ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + ii] = 1.e-30f;  
							// }
							// else
								ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + ii] = par43[ii];
						   }


						// // tiesiog visa zona cx4345
						// ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + ii] = par4543[ii];
						// // // tiesiog visa zona cx43
						// // ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + ii] = par43[ii];
					}

					// if (isNonCond) {
					// 	float gjOpen = 1.e-30f;
					// 	float gjClosed = 1.e-30f;
					// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 5] = gjOpen;	  //Go1 set anis
					// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 6] = gjClosed;  //Gc1 set anis
					// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 12] = gjOpen;	  //Go2 set anis
					// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 13] = gjClosed; //Gc2 set anis
					// }

					// NW junciu apsukimas, kad tolygiai einu left->right cx43->cx45, (zr. uzrasus "zadinimo eiliskumas") 
					if (i == 1) { // NW
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 3] *= -1.f; // V01
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 4] *= -1.f; // Pol1
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 10] *= -1.f; // V02
						ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 11] *= -1.f; // Pol2
					}

					// if (orientation) {  // orientacijos erdveje issibarstymas
					// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 3] *= -1.f; // V01
					// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 4] *= -1.f; // Pol1
					// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 10] *= -1.f; // V02
					// 	ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + i * 7 * 2 + 11] *= -1.f; // Pol2
					// }
				}
					// printf("%d ", rand());
				int sel = 0;																	// select W
				ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 5] *= A;	//Go1 set anis
				ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 6] *= A;	//Gc1 set anis
				ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 12] *= A; //Go2 set anis
				ssb_gj_par_host[y * sizeX * 3 * 7 * 2 + x * 3 * 7 * 2 + sel * 7 * 2 + 13] *= A; //Gc2 set anis
			}

			//for (int i = 0; i < 1; i++)
			//ssb_dbg_host[y * sizeX * 1 + x * 1 + i] = 0;
		}
	for (int i = 0; i < frame_chunk_size; i++)
	{
		for (int y=0; y < sizeY; y++)
			for (int x=0; x < sizeX; x++) {
				ssb_u_reg_host[i*sizeX*sizeY + y*sizeX + x] = y*(i+1);//.f; //sin(float(i) / 100);
				ssb_gj_reg_host[i*sizeX*sizeY*3 + y*sizeX*3 + x*3 + 0] = 0;//i+1;
				ssb_gj_reg_host[i*sizeX*sizeY*3 + y*sizeX*3 + x*3 + 1] = 0;//2*(i+1);
				ssb_gj_reg_host[i*sizeX*sizeY*3 + y*sizeX*3 + x*3 + 2] = 0;//3*(i+1);
			}
		// ssb_dbg_host[i * dbg_size + 0] = 0.f;
		// ssb_dbg_host[i * dbg_size + 1] = 0.f;
		// ssb_dbg_host[i * dbg_size + 2] = 0.f;
		// ssb_dbg_host[i * dbg_size + 3] = 0.f;
		// ssb_dbg_host[i * dbg_size + 4] = 0.f;
		// ssb_dbg_host[i * dbg_size + 5] = 0.f;
		// ssb_dbg_host[i * dbg_size + 6] = 0.f;
		// ssb_dbg_host[i * dbg_size + 7] = 0.f;
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
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeX * sizeY * sizeof(ssb_u_host[0]), ssb_u_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_u);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_v);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeX * sizeY * sizeof(ssb_v_host[0]), ssb_v_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_v);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_w);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeX * sizeY * sizeof(ssb_w_host[0]), ssb_w_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_w);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_J_ion);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeX * sizeY * sizeof(ssb_J_ion_host[0]), ssb_J_ion_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_J_ion);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_J_gj);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * sizeX * sizeY * sizeof(ssb_J_gj_host[0]), ssb_J_gj_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_J_gj);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u_reg);
	glBufferData(GL_SHADER_STORAGE_BUFFER, frame_chunk_size * sizeX*sizeY * sizeof(ssb_u_reg_host[0]), ssb_u_reg_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssbo_u_reg);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * sizeX * sizeY * sizeof(ssb_gj_host[0]), ssb_gj_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, ssbo_gj);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_reg);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * frame_chunk_size * sizeX*sizeY * sizeof(ssb_gj_reg_host[0]), ssb_gj_reg_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, ssbo_gj_reg);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_par);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(ssb_gj_par_host[0]), ssb_gj_par_host, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 12, ssbo_gj_par);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_p);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 4 * sizeX * sizeY * sizeof(ssb_gj_p_host[0]), ssb_gj_p_host, GL_DYNAMIC_COPY);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 13, ssbo_gj_p);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_dbg);
	glBufferData(GL_SHADER_STORAGE_BUFFER, frame_chunk_size * sizeof(ssb_dbg_host[0]), ssb_dbg_host, GL_DYNAMIC_COPY); //sizeof(data) only works for statically sized C/C++ arrays.
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 15, ssbo_dbg);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); //unbind


	glUseProgram(kernel1); //glUseProgram(shIDs.compID1);
	//GLint location = glGetUniformLocation(shIDs.compID1, "sizeXY"); printf("location %d \n", location);	if (location == -1) { printf("Could not locate uniform location in CS. "); }
	glUniform2i(20, sizeX, sizeY);
	//location = glGetUniformLocation(shIDs.compID1, "dt_sim"); printf("location %d \n", location);	if (location == -1) { printf("Could not locate uniform location in CS. "); }
	glUniform1f(21, par.dt); // kai float
	// glUniform1d(21, par.dt); // kai double
	GLint location = glGetUniformLocation(kernel1, "gjModelEnabled"); //galima ir su location
	glUniform1i(location, enableGjModel);
	glUseProgram(kernel2); //glUseProgram(shIDs.compID2);
	glUniform2i(20, sizeX, sizeY);
	// glUniform1d(21, par.dt); //kai double
	glUniform1f(21, par.dt); //kai float
	glUseProgram(vertFragShadersID); //glUseProgram(shIDs.frVeID);
	glUniform2i(20, sizeX, sizeY);
	glUseProgram(line2DvertFragShadersID); //glUseProgram(shIDs.frVeID);
	glUniform2i(20, sizeX, sizeY);
	glUniform1ui(glGetUniformLocation(line2DvertFragShadersID, "frame_chunk_size"), frame_chunk_size);

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

	// Zarr
	// std::filesystem::remove_all("data1.zr");
	std::filesystem::remove_all("d:\data_tmp1.zr");
	// z5::filesystem::handle::File f("data1.zr", z5::FileMode::modes::w);
	z5::filesystem::handle::File f("d:\data_tmp1.zr", z5::FileMode::modes::w);

	// create the file in zarr format
	const bool createAsZarr = true;
	z5::createFile(f, createAsZarr);

	// create a new zarr dataset
	const size_t i_frame_max = t_max * 1. / par.dt / itersInFrame; //6000e3; // dt*iter_in_frame= 0.02*10=0.2 ms per frame

	const std::string dsName = "u";
	std::vector<size_t> shape = {i_frame_max, sizeY, sizeX};
	std::vector<size_t> chunks = {i_frame_max / 100 + 1, sizeY, sizeX};
	auto u_ds = z5::createDataset(f, dsName, "float32", shape, chunks);

	const std::string dsName1 = "gj";
	std::vector<size_t> shape1 = {i_frame_max, sizeY, sizeX, 3};
	std::vector<size_t> chunks1 = {i_frame_max / 100 + 1, sizeY, sizeX, 3};
	auto gj_ds = z5::createDataset(f, dsName1, "float32", shape1, chunks1);

	// get handle for the dataset
	const auto dsHandle = z5::filesystem::handle::Dataset(f, dsName);
	const auto dsHandle1 = z5::filesystem::handle::Dataset(f, dsName1);

	// read and write json attributes
	nlohmann::json attributesIn;
	// attributesIn["bar"] = "foo";

	uint32_t i = 0;
	uint32_t i_frame = 0;
	auto current_frame_chunk = 0;
	t_start = clock();
	do // main Loop
	{
		if (!pause)
		{
			mtx.lock();
			glfwMakeContextCurrent(window);			
			glClear(GL_COLOR_BUFFER_BIT);
			//Sleep(50);
			// printf("\rframe %d, iter %d, simul %.2f ms", i_frame, i, i * .02f);
			auto i_frame_in_current_chunk = i_frame % frame_chunk_size;
			// printf("\riter %d, iter_in_chunk %d, simul %.2f ms", i, i_frame_in_current_chunk, i * .02f);
			for (int a = 0; a < itersInFrame; a++) //i loop
			{ // launch compute shaders!
				// glMemoryBarrier(GL_ALL_BARRIER_BITS);
				glUseProgram(kernel1); //glUseProgram(shIDs.compID1);
				glUniform1ui(22, i);   //butina, mc36ss, ---nebutina, debug
				glUniform1ui(25, i_frame_in_current_chunk); // i_frame_in_current_chunk reikia gj_reg isvedimui
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				// glDispatchCompute((GLuint)(sizeX + workGroupSize) / workGroupSize, (GLuint)(sizeY + workGroupSize) / workGroupSize, 1); // + workGroupSize - 1) tam kad butu "divide round up"
				glDispatchCompute((GLuint)(sizeX + workGroupSizeX -1) / workGroupSizeX, (GLuint)(sizeY + workGroupSizeY -1) / workGroupSizeY, 1); // SVARBU, bug fixed : .. - 1 nereikia, nes tada uzsiriecia prie periodic boundaries
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				glUseProgram(kernel2); //glUseProgram(shIDs.compID2);
				glUniform1ui(22, i);
				glUniform1ui(25, i_frame_in_current_chunk);// reikia u_reg isvedimui
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				// glDispatchCompute((GLuint)(sizeX + workGroupSize) / workGroupSize, (GLuint)(sizeY + workGroupSize) / workGroupSize, 1);
				glDispatchCompute((GLuint)(sizeX + workGroupSizeX-1 ) / workGroupSizeX, (GLuint)(sizeY + workGroupSizeY -1) / workGroupSizeY, 1);
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


			if ((i_frame + 1) % frame_chunk_size == 0)
			{
			printf("\riter %d, iter_in_chunk %d, simul %.2f ms", i, i_frame_in_current_chunk, i * .02f);				
			// perkelta update kas chunk
			glUseProgram(vertFragShadersID); //;glUseProgram(shIDs.frVeID);
			//glPointSize(5.f);
			// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //wireframe mode
			glBindVertexArray(VAO);
			glUniform1ui(26, 1); //whatToPlot: 1 u, 2 vj, 3 gj
			glDrawElements(GL_TRIANGLES, (sizeX - 1) * (sizeY - 1) * 2 * 3, GL_UNSIGNED_INT, 0);
			glUniform1ui(26, 2); //whatToPlot: 1 u, 2 vj, 3 gj
			glDrawElements(GL_TRIANGLES, (sizeX - 1) * (sizeY - 1) * 2 * 3, GL_UNSIGNED_INT, 0);
			glUniform1ui(26, 3); //whatToPlot: 1 u, 2 vj, 3 gj
			glDrawElements(GL_TRIANGLES, (sizeX - 1) * (sizeY - 1) * 2 * 3, GL_UNSIGNED_INT, 0);

			// Draw line graphs
			glUseProgram(line2DvertFragShadersID);
			glUniform1ui(25, i_frame_in_current_chunk);
			glUniform1ui(26, 1); //whatToPlot: 1 u, 2 gj
			// glMemoryBarrier(GL_ALL_BARRIER_BITS);
			glBindVertexArray(VAO2);
			glDrawArrays(GL_LINE_STRIP, 0, line_points_count);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			glUniform1ui(26, 2); //whatToPlot: 1 u, 2 gj
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			glDrawArrays(GL_LINE_STRIP, 0, line_points_count);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			// END perkelta update kas chunk

				size_t last_iter = current_frame_chunk * frame_chunk_size;

				glMemoryBarrier(GL_ALL_BARRIER_BITS);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u_reg);
				glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, frame_chunk_size * sizeX * sizeY * sizeof(ssb_u_reg_host[0]), ssb_u_reg_host);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_reg);
				glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, frame_chunk_size * sizeX * sizeY * 3 * sizeof(ssb_gj_reg_host[0]), ssb_gj_reg_host);
				glMemoryBarrier(GL_ALL_BARRIER_BITS);

std::async([&] {
				attributesIn["frames_count"] = last_iter;
				attributesIn["dt_reg"] = itersInFrame * par.dt;
				z5::writeAttributes(dsHandle, attributesIn);
				z5::writeAttributes(dsHandle1, attributesIn);

				z5::types::ShapeType offset = {last_iter, 0, 0};
				auto size = frame_chunk_size * sizeY * sizeX;
				std::vector<std::size_t> shape = {frame_chunk_size, sizeY, sizeX};
				auto array = xt::adapt(ssb_u_reg_host, size, xt::no_ownership(), shape);
				z5::multiarray::writeSubarray<float>(u_ds, array, offset.begin());

				z5::types::ShapeType offset1 = {last_iter, 0, 0, 0};
				auto size1 = frame_chunk_size * sizeY * sizeX * 3;
				std::vector<std::size_t> shape1 = {frame_chunk_size, sizeY, sizeX, 3};
				auto array1 = xt::adapt(ssb_gj_reg_host, size1, xt::no_ownership(), shape1);
				z5::multiarray::writeSubarray<float>(gj_ds, array1, offset1.begin());

				printf("\ncurrent_frame_chunk: %d \n", current_frame_chunk);
				printf("\n***iter_in_chunk %d, simul %.2f ms\n", i_frame_in_current_chunk, i * .02f);
});				

				current_frame_chunk++;


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
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(ssb_gj_par_host[0]), ssb_gj_par_host, GL_DYNAMIC_COPY);
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
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(ssb_gj_par_host[0]), ssb_gj_par_host, GL_DYNAMIC_COPY);
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
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(ssb_gj_par_host[0]), ssb_gj_par_host, GL_DYNAMIC_COPY);
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
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(ssb_gj_par_host[0]), ssb_gj_par_host, GL_DYNAMIC_COPY);
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
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(ssb_gj_par_host[0]), ssb_gj_par_host, GL_DYNAMIC_COPY);
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
				glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * 2 * 7 * sizeX * sizeY * sizeof(ssb_gj_par_host[0]), ssb_gj_par_host, GL_DYNAMIC_COPY);
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
						}
			i_frame++;// perkelta update kas chunk						
			glfwMakeContextCurrent(NULL);
			mtx.unlock();

			if (i >= t_max/par.dt)
				//i_frame = 0;
				break;
			// if (i_frame < i_frame_max)
			// 	i_frame++;
			// else
			// 	//i_frame = 0;
			// 	break;
			std::this_thread::sleep_for(std::chrono::microseconds(100));

		} // main loop

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
				glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeX * sizeY * sizeof(ssb_u_host[0]), ssb_u_host);
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
	printf("... nDone.\n");
	t_end = clock();

	glfwMakeContextCurrent(window);
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj);
	// glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 3 * sizeX * sizeY * sizeof(ssb_gj_host[0]), ssb_gj_host);
	// glMemoryBarrier(GL_ALL_BARRIER_BITS);

	// try
	// {
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u);
	// glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeX * sizeY * sizeof(ssb_u_host[0]), ssb_u_host);
	// glMemoryBarrier(GL_ALL_BARRIER_BITS);
	// 		}
	// catch (const std::exception &exc)
	// {
	// 	std::cerr << exc.what();
	// 	// std::cout << exc.what();
	// }
	
	
	// for (int i = 0; i < sizeX * sizeY ; i++)
	// 	printf("%d ", ssb_u_host[i]);

	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_gj_reg);
	// glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, 3 *i_frame_max* sizeX*sizeY *  sizeof(ssb_gj_reg_host[0]), ssb_gj_reg_host);
	// glMemoryBarrier(GL_ALL_BARRIER_BITS);



	//
	//export data to file
	//
	// csv file
	// glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_dbg);
	// glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, dbg_size * i_frame_max * sizeof(ssb_dbg_host[0]), ssb_dbg_host);
	// glMemoryBarrier(GL_ALL_BARRIER_BITS);	
	// std::ofstream myfile;
	// myfile.open("reg.csv");
	// myfile << "t,v1,v2,v3,v4,v5,v6,v7,gj1,gj2,gj3\n";
	// for (int i = 0; i < i_frame; i++)
	// 	myfile << i * itersInFrame * par.dt
	// 	 	  //TODO: dbg_ --->> pakeist i u_reg
	// 		   << "," << ssb_dbg_host[i * dbg_size + 0] //v1
	// 		   << "," << ssb_dbg_host[i * dbg_size + 1] //v2
	// 		   << "," << ssb_dbg_host[i * dbg_size + 2] //v3
	// 		   << "," << ssb_dbg_host[i * dbg_size + 3] //v4
	// 		   << "," << ssb_dbg_host[i * dbg_size + 4] //v5
	// 		   << "," << ssb_dbg_host[i * dbg_size + 5] //v6
	// 		   << "," << ssb_dbg_host[i * dbg_size + 7] //v7
	// 		   << "," << ssb_gj_reg_host[0 * i_frame_max + i] 
	// 		   << "," << ssb_gj_reg_host[1 * i_frame_max + i]
	// 		   << "," << ssb_gj_reg_host[2 * i_frame_max + i]
	// 		   << std::endl;
	// myfile.close();
	// printf("\n\nResults file 'reg.csv' wrote.");


	// writes u_reg[iframe*sizeX*sizeX] ir gj_reg[iframe*sizeX*sizeX]
    // std::vector<float_t> data(ssb_u_reg_host, ssb_u_reg_host + sizeof ssb_u_reg_host / sizeof ssb_u_reg_host[0]);
	// size_t bytes = sizeof(data[0]) * data.size();
    // auto binFile = std::fstream("reg.binary", std::ios::out | std::ios::binary);
    // binFile.write((char*)&data[0], bytes);
    // binFile.close();
	
	// std::ofstream out;
	// out.open( "reg.binary", std::ios::out | std::ios::binary);
	// float f[3] = {1.1,2.2,3.2};
	// out.write( reinterpret_cast<const char*>( &f ), sizeof f / sizeof f[0]);
	// out.close();	
	// FILE *fp;
	// fp = fopen("reg.binary", "wb");
	// fwrite(reinterpret_cast<char*>(ssb_u_reg_host), 1, i_frame_max * sizeX*sizeY * sizeof(float), fp);
	// fclose(fp);
	// std::vector<float> data(ssb_u_reg_host, ssb_u_reg_host + sizeof ssb_u_reg_host / sizeof ssb_u_reg_host[0]);


	// // Binary files

	// unsigned uptoFrame = i_frame;
	// //write Voltage
	// printf("Writing file 'reg_u_xy_iFrame.npy'...");
	// std::vector<float> data;//(std::begin(*ssb_u_reg_host), std::end(*ssb_u_reg_host));
	// data.assign(ssb_u_reg_host, ssb_u_reg_host+uptoFrame*sizeY*sizeX);
	// const long unsigned leshape [] = {uptoFrame,sizeY,sizeX};
  	// npy::SaveArrayAsNumpy("reg_u_xy_iFrame.npy", false, 3, leshape, data);
	// printf(" Done.\n"); 	

	// // //write GJ - 1d array
	// // printf("Writing file 'reg_gj_xy_iFrame_orientation.npy'...");
	// // std::vector<float> data2;//(std::begin(*ssb_u_reg_host), std::end(*ssb_u_reg_host));
	// // data2.assign(ssb_gj_reg_host, ssb_gj_reg_host+3*uptoFrame*sizeY*sizeX);
	// // const long unsigned leshape2 [] = {uptoFrame*sizeY*sizeX*3};
  	// // npy::SaveArrayAsNumpy("reg_gj_xy_iFrame_orientation.npy", false, 1, leshape2, data2);
	// // printf(" Done.\n,%d", uptoFrame);

		
	// printf("Writing file 'reg_gj_xy_iFrame_orientation.npy'...");
	// // float arr[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24};
	// // std::vector<float> data2 = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24};//(std::begin(*ssb_u_reg_host), std::end(*ssb_u_reg_host));
	// std::vector<float> data2 {};//(std::begin(*ssb_u_reg_host), std::end(*ssb_u_reg_host));
	// // data2.assign(arr, arr+24);
	// data2.assign(ssb_gj_reg_host, ssb_gj_reg_host + (uptoFrame+1)*sizeY*sizeX*3);
	// // data2.assign(ssb_gj_reg_host, ssb_gj_reg_host+(uptoFrame+1)*sizeY*sizeX*3-1);
	// const long unsigned leshape2 [] = {uptoFrame+1,sizeY,sizeX,3};
	// // const long unsigned leshape2 [] = {2,2,2,3};
  	// npy::SaveArrayAsNumpy("reg_gj_orientation_xy_iFrame.npy", false, 4, leshape2, data2);
	// printf(" Done.\n");

	// printf("\nLast frame: %d \n", uptoFrame);


		// auto i_frame_in_last_chunk = 33;   // kiek i_frame itaraciju paskutiniam chunk (svarbu, nes gali buti sustabdytas viduty skaiciavimo)
		// // write array to roi
		// auto last_iter = current_frame_chunk * frame_chunk_size + i_frame_in_last_chunk;
		
		// z5::types::ShapeType offset1 = {0, 0, 0};

		// auto size = i_frame_in_last_chunk * sizeY * sizeX;
		// std::vector<std::size_t> shape = {i_frame_in_last_chunk, sizeY, sizeX};
		// auto array1 = xt::adapt(ssb_u_reg_host, size, xt::no_ownership(), shape);
		// z5::multiarray::writeSubarray<float>(ds, array1, offset1.begin());



	// cout << "zarr done\n";

	// std::ofstream myfile2;
	// myfile2.open("dbg.csv");
	// myfile2 << "i,dbg\n";
	// for (int i = 0; i < 20; i++) {
	// 	myfile2 << i << ",";
	// 	for (int k = 0; k < 16; k++)
	// 		myfile2 << ssb_dbg_host[i*16+k] << ",";
	// 	myfile2 << std::endl;
	// }
	// myfile2.close();
	// printf("\n\nfile 'dbg.csv' wrote.");

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
		   (float)i / itersInFrame / execTime);
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
	exit(1);
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
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeX * sizeY * sizeof(ssb_u_host[0]), ssb_u_host);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		// glClear(GL_COLOR_BUFFER_BIT);
		// std::this_thread::sleep_for(std::chrono::seconds(3));
		glfwMakeContextCurrent(NULL);
		mtx.unlock();
		cout << "\nUnlocked\n";
		float *Vm = ssb_u_host; // kai float
		// double *Vm = ssb_u_host; // kai double

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
	std::string server_address("0.0.0.0:8002");
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