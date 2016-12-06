set(SDL_DIR "C:/local/SDL/SDL2-2.0.4/")

find_path(SDL_INCLUDE_DIRS
	NAMES
		SDL.h
	PATHS
		$ENV{WIN_LOCAL_DIR}/SDL/SDL2-2.0.4/include
		"${SDL_DIR}/include"
		/opt/local/include/SDL2
		/usr/local/include/SDL2
)

find_library(SDL_LIBRARIES
	NAMES
		SDL2
	PATHS
		$ENV{WIN_LOCAL_DIR}/SDL/SDL2-2.0.4/lib/x64
		"${SDL_DIR}/lib/x64"
		/Library/Frameworks
)

message("SDL2 Library: ${SDL_LIBRARIES}")

