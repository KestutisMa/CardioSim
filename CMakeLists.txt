# list(APPEND CMAKE_PREFIX_PATH "C:/Miniconda3/envs/py38")
message("**********************\ncmake PrefixPath\n*****************" ${CMAKE_PREFIX_PATH})

cmake_minimum_required(VERSION 3.0.0)
project(fentonGjOpenGL VERSION 0.1.0)
# set_property(TARGET fentonGjOpenGL PROPERTY CXX_STANDARD 20)
# set_property(TARGET fentonGjOpenGL PROPERTY CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# find_package(absl CONFIG REQUIRED)
find_package(gRPC REQUIRED)
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
include_directories(C:/OpenGL/glfw-3.3.2.bin.WIN64/include/
 C:/OpenGL/glew-2.1.0-win32/glew-2.1.0/include/
#  C:/Miniconda3/pkgs/z5py-2.0.11-py38h6790dec_0/Library/include
#  C:/Miniconda3/pkgs/xtensor-0.24.0-h5362a0b_0/Library/include
#  C:/OpenGL/fentonGjOpenGL/vcpkg/packages/python3_x64-windows/include/python3.7
#  ${xtensor_INCLUDE_DIRS}
#  C:/Miniconda3/envs/py38/Library/include/nlohmann
# ${nlohmann_INCLUDE_DIRS}
 ${PYTHON_INCLUDE_DIRS}
 ${Protobuf_INCLUDE_DIRS}
 ${CMAKE_CURRENT_BINARY_DIR}
#  ${Boost_INCLUDE_DIR}
#  C:/OpenGL/fentonGjOpenGL/protos/
 )
link_directories(C:/OpenGL/glfw-3.3.2.bin.WIN64/lib-vc2019/

    C:/OpenGL/glew-2.1.0-win32/glew-2.1.0/lib/Release/x64/)
# enable_testing(C:/OpenGL/glew-2.1.0-win32/glew-2.1.0/)

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
        --proto_path="C:/Miniconda3/envs/py38/Library/include/"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")
add_executable(fentonGjOpenGL main.cpp shader.cpp ${hw_proto_srcs} ${hw_grpc_srcs}) # socketsServerExample.cpp)
# add_executable(helloWorldServer HelloWorld_Server/HelloWorld_Server.cpp ${hw_proto_srcs} ${hw_grpc_srcs}) # socketsServerExample.cpp)
# add_executable(helloWorldClient HelloWorld_Client/HelloWorld_Client.cpp ${hw_proto_srcs} ${hw_grpc_srcs}) # socketsServerExample.cpp)
add_executable(mc4sm mc4sm.cpp matrix_exponential.cpp r8lib.cpp) # socketsServerExample.cpp)
target_link_libraries(fentonGjOpenGL PRIVATE glfw3dll glew32 opengl32 Ws2_32 gRPC::grpc++ xtensor nlohmann_json::nlohmann_json )# ${Protobuf_LIBRARIES})      
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

