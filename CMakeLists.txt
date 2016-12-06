cmake_minimum_required(VERSION 3.6)

project(binview LANGUAGES C)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL REQUIRED)

include_directories(${SDL_INCLUDE_DIR})
include_directories(${GLEW_INCLUDE_DIRS})

add_executable(binview binview.c)

target_link_libraries(binview
	${SDL_LIBRARY}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})


