cmake_minimum_required(VERSION 3.6)

project(binview LANGUAGES C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL REQUIRED)

include_directories(${SDL_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})

add_executable(binview binview.c)

target_link_libraries(binview
	${SDL_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})


