IF (WIN32)
  list(APPEND CMAKE_PREFIX_PATH "C:/Miniconda3/envs/py38") #butina kad nekonfliktuotu su system libraries
ELSE()
  # list(APPEND CMAKE_PREFIX_PATH "/opt/miniconda3/") #butina kad nekonfliktuotu su system libraries
  list(APPEND CMAKE_PREFIX_PATH "/home/lab/.conda/envs/py39/") #butina kad nekonfliktuotu su system libraries
ENDIF()
message("****\ncmake PrefixPath = " ${CMAKE_PREFIX_PATH})

cmake_minimum_required(VERSION 3.0.0)
project(fentonGjOpenGL VERSION 0.1.0)
# set_property(TARGET fentonGjOpenGL PROPERTY CXX_STANDARD 20)
# set_property(TARGET fentonGjOpenGL PROPERTY CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

# if(MSVC)
#   set(CMAKE_EXE_LINKER_FLAGS
#       "${CMAKE_EXE_LINKER_FLAGS}  /MD") #/verbose:lib
# endif()

# include(FetchContent)
# FetchContent_Declare(
#   gRPC
#   GIT_REPOSITORY https://github.com/grpc/grpc
#   GIT_TAG        v1.32.0  # e.g v1.28.0
# )
# set(FETCHCONTENT_QUIET OFF)
# FetchContent_MakeAvailable(gRPC)

option(WITH_BLOSC "Build with blosc compression" OFF)

# find_package(absl CONFIG REQUIRED)
find_package(gRPC REQUIRED)
find_package(OpenMP REQUIRED)
# find_package(BLOSC REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(Protobuf REQUIRED)
# find_package(gRPC CONFIG REQUIRED)
# FIND_PACKAGE(Boost REQUIRED)
# find_package(PythonLibs 3.7 REQUIRED)
# find_package(Boost REQUIRED COMPONENTS python37)
# find_package(Python 3 REQUIRED COMPONENTS Interpreter)

find_package(xtensor CONFIG REQUIRED)
# find_package(xtensor-zarr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
# message("**********************\n json \n*****************" ${xtensor_INCLUDE_DIRS})

# include(CTest)
IF (WIN32)
  include_directories(C:/OpenGL/glfw-3.3.2.bin.WIN64/include/
  C:/OpenGL/glew-2.1.0-win32/glew-2.1.0/include/
  ${PYTHON_INCLUDE_DIRS}
  ${Protobuf_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
ELSE()
  include_directories(
  ${PYTHON_INCLUDE_DIRS}
  # ${Protobuf_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )
ENDIF()

IF (WIN32)
  link_directories(C:/OpenGL/glfw-3.3.2.bin.WIN64/lib-vc2019/
    C:/OpenGL/glew-2.1.0-win32/glew-2.1.0/lib/Release/x64/)
ELSE()
  # link_directories(/opt/miniconda3/lib)
  link_directories(/home/lab/.conda/envs/py39/lib)
ENDIF()

find_program(_PROTOBUF_PROTOC protoc)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)

get_filename_component(hw_proto "fentonControl.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)
# message("**********************\nhello\n*****************" ${hw_proto_path})
# message("**********************\nenv\n*****************" $ENV{CONDA_PREFIX})
message("**********************\n protoc *****************" ${_PROTOBUF_PROTOC})
# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/fentonControl.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/fentonControl.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/fentonControl.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/fentonControl.grpc.pb.h")
add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        # --proto_path="C:/vcpkg/installed/x64-windows/include"
        # --proto_path="C:/Miniconda3/envs/py38/Library/include/"
        --proto_path="/opt/miniconda3/bin/protoc"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")
add_executable(fentonGjOpenGL main.cpp shader.cpp read_ppm.hpp ${hw_proto_srcs} ${hw_grpc_srcs}) # socketsServerExample.cpp)
# add_executable(helloWorldServer HelloWorld_Server/HelloWorld_Server.cpp ${hw_proto_srcs} ${hw_grpc_srcs}) # socketsServerExample.cpp)
# add_executable(helloWorldClient HelloWorld_Client/HelloWorld_Client.cpp ${hw_proto_srcs} ${hw_grpc_srcs}) # socketsServerExample.cpp)

# add_executable(mc4sm mc4sm.cpp matrix_exponential.cpp r8lib.cpp) # socketsServerExample.cpp)
IF (WIN32)
  target_link_libraries(fentonGjOpenGL PRIVATE glfw3dll glew32 opengl32 Ws2_32 gRPC::grpc++ xtensor nlohmann_json::nlohmann_json )# ${Protobuf_LIBRARIES})        
ELSE()
  target_link_libraries(fentonGjOpenGL PRIVATE blosc glfw GLEW GL gRPC::grpc++ xtensor nlohmann_json::nlohmann_json OpenMP::OpenMP_CXX)# ${Protobuf_LIBRARIES})      
ENDIF()

#Mano Boost lib
#reikia:
# setlocal
# neveikia SET PATH=%PATH%;C:\OpenGL\fentonGjOpenGL\vcpkg\installed\x64-windows\bin;C:\OpenGL\fentonGjOpenGL\vcpkg\packages\python3_x64-windows\debug\bin
# veikia SET PATH=%PATH%;C:\OpenGL\fentonGjOpenGL\vcpkg\installed\x64-windows\debug\bin
 
# SET(Boost_USE_STATIC_LIBS     OFF)
# SET(Boost_USE_MULTITHREADED    ON)
# SET(Boost_USE_STATIC_RUNTIME     OFF)
# PYTHON_ADD_MODULE(mc4sm_lib boost_greet.cpp)
## add_library(mc4sm_lib MODULE boost_greet.cpp )
# set_target_properties(mc4sm_lib PROPERTIES SUFFIX .pyd)
# target_link_libraries(mc4sm_lib ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} C:/OpenGL/fentonGjOpenGL/vcpkg/packages/python3_x64-windows/lib/python37.lib)
# add_custom_command(TARGET mc4sm_lib 
#                    POST_BUILD
#                    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mc4sm_lib> C:/Miniconda3/Lib/site-packages/)

# add_executable(socketsServerExample socketsServerExample.cpp)
# target_link_libraries(fentonGjOpenGL PRIVATE glfw3dll glew32 opengl32 Ws2_32 gRPC::grpc++ xtensor xtensor-zarr)# ${Protobuf_LIBRARIES})
# target_link_libraries(fentonGjOpenGL PRIVATE glfw3dll glew32 opengl32 Ws2_32 gRPC::grpc++ xtensor nlohmann_json nlohmann_json::nlohmann_json)# ${Protobuf_LIBRARIES})
# target_link_libraries(main PRIVATE xtensor xtensor::use_TBB xtensor::optimize xtensor::use_xsimd)
# target_link_libraries(main PRIVATE xtensor xtensor::use_TBB xtensor::optimize xtensor::use_xsimd)
# target_link_libraries(helloWorldServer gRPC::grpc++)
# target_link_libraries(helloWorldClient gRPC::grpc++)

# get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
# foreach(dir ${dirs})
#   message(STATUS "dir='${dir}'")
# endforeach()

# set(CPACK_PROJECT_NAME ${PROJECT_NAME})
# set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)

