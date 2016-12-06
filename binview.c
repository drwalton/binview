#include <GL/glew.h>
#include <SDL.h>
#ifdef WIN32
#undef main
#endif
#include <stdio.h>

int width = 200, height = 600;
const int maxScrollLines = 20;
const float verts[] = {
	-1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f, 1.f, -1.f, -1.f, 1.f, -1.f };
const float texCoords[] = {
	0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f };

const GLchar *const vertShaderSource =
"#version 410\n"
"\n"
"layout(location = 0) in vec2 vPos;\n"
"layout(location = 1) in vec2 vTex;\n"
"\n"
"out vec2 tex;\n"
"\n"
"void main() {\n"
"	tex = vTex;\n"
"	gl_Position = vec4(vPos, 0, 1);\n"
"}\n"
;
const GLchar *const fragShaderSource =
"#version 410\n"
"\n"
"in vec2 tex;\n"
"\n"
"uniform sampler2D texSampler;\n"
"\n"
"out vec3 color;\n"
"\n"
"void main() {\n"
"	color = texture(texSampler, tex).rgb;\n"
"//	color = vec4(tex, 0, 1);\n"
"}\n"
;

int min(int a, int b) {
	return a < b ? a : b;
}

GLboolean eventIsQuit(SDL_Event e)
{
	if (e.type == SDL_QUIT) {
		return GL_TRUE;
	}
	if (e.type == SDL_WINDOWEVENT) {
		if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
			return GL_TRUE;
		}
	}
	return GL_FALSE;
}

int compileAndCheckShader(GLuint shader)
{
	glCompileShader(shader);
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) //Compilation failure.
	{
		GLint logLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		GLchar* errorLog = malloc(logLength);
		glGetShaderInfoLog(shader, logLength, 0, errorLog);
		printf("Failed to compile shader: %s\n", errorLog);
		free(errorLog);
		return GL_FALSE;
	}
	return GL_TRUE;
}
int linkAndCheckProgram(GLuint program)
{
	glLinkProgram(program);
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) //Compilation failure.
	{
		GLint logLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		GLchar* errorLog = malloc(logLength);
		glGetProgramInfoLog(program, logLength, 0, errorLog);
		printf("Failed to link shaders: %s\n", errorLog);
		free(errorLog);
		return GL_FALSE;
	}
	return GL_TRUE;
}

const GLbyte BLACK[3] = {0, 0, 0};
const GLbyte BLUE[3] = {0, 0, 255};
const GLbyte RED[3] = {255, 0, 0};

void byteToColor(GLbyte byte, GLbyte *color) {
	if(byte == 0) memcpy(color, BLACK, 3);
	else if(byte < 127) memcpy(color, BLUE, 3);
	else memcpy(color, RED, 3);
}

int main(int argc, char *argv[])
{
	if(argc < 2) {
		printf("Usage: binview [filename] <width> <height>\n");
		return 1; //No filename supplied.
	}
	printf("Loading file: %s\n", argv[1]);
	if(argc >= 4) {
		width = atoi(argv[2]);
		height = atoi(argv[3]);
	}
	FILE *file = fopen(argv[1], "rb");

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		return -1; //unable to init SDL.
	}
	
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	
	Uint32 winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
	
	SDL_Window *win = SDL_CreateWindow("binview", 30, 30,
		width, height, winFlags);
	SDL_GLContext context = SDL_GL_CreateContext(win);
	SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, 0);
	SDL_GL_MakeCurrent(win, context);
	
	glewExperimental = GL_TRUE;
	{
    	GLenum r = glewInit();
    	if (r != GLEW_OK) {
    		return -2; //unable to init glew.
    	}
	}
	if (!glewIsSupported("GL_VERSION_4_1")) {
		return -3; //opengl_4_1 not supported.
	}
	
	//Make texture, and populate with initial chunk of file.
	GLuint texture;
	{
    	GLbyte *fileChunk = malloc(width*height);
		memset(fileChunk, 0, width*height);
    	GLbyte *texData = malloc(width*height*3);
		fread(fileChunk, 1, width*height, file);
		for(int r = 0; r < height; ++r) {
			for(int c = 0; c < width; ++c) {
				byteToColor(fileChunk[(height-r-1)*width + c], texData + (r*width + c)*3);
			}
		}
		for(int i = 0; i < width*height; ++i)
			byteToColor(fileChunk[i], texData + ((width*height-i)*3));
    	glGenTextures(1, &texture);
    	glBindTexture(GL_TEXTURE_2D, texture);
    	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
    		width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	
	//Make vertex buffers.
	GLuint vao, vertBuffer, texCoordBuffer;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(float), verts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glGenBuffers(1, &texCoordBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(float), texCoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
	//Create shaders.
	GLuint vertShader, fragShader, program;
	vertShader = glCreateShader(GL_VERTEX_SHADER);
	fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertShader, 1, &vertShaderSource, 0);
	glShaderSource(fragShader, 1, &fragShaderSource, 0);
	if(!compileAndCheckShader(vertShader) ||
	   !compileAndCheckShader(fragShader)) {
		return -4; //Shaders failed to compile.
	}
	program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	if(!linkAndCheckProgram(program)) {
		return -5; //Shaders failed to link.
	}
	
	SDL_Event event;
	GLboolean running = GL_TRUE;
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(vao);
	glUseProgram(program);
	GLuint texSamplerUniform = glGetUniformLocation(program, "texSampler");
	glProgramUniform1i(program, texSamplerUniform, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	SDL_GL_SwapWindow(win);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	
	GLbyte *fileBuff = malloc(maxScrollLines*width);
	GLbyte *texBuff = malloc(maxScrollLines*width*3);
	glReadBuffer(GL_FRONT);
	
	while(running) {
		while(SDL_PollEvent(&event)) {
			if(eventIsQuit(event)) {
				running = GL_FALSE;
			}
			
			if(event.type == SDL_MOUSEWHEEL) {
				if(event.wheel.y > 0) {
					//Scrolled up.
					//TODO
				} else if(event.wheel.y < 0) {
					//Scrolled down.
					int linesToLoad = min(-event.wheel.y, maxScrollLines);
					if(!feof(file)) {
						fread(fileBuff, 1, width*linesToLoad, file);
                		for(int r = 0; r < linesToLoad; ++r) {
                			for(int c = 0; c < width; ++c) {
                				byteToColor(
									fileBuff[(linesToLoad-r-1)*width + c],
									texBuff + (r*width + c)*3);
                			}
                		}
						for(size_t i = 0; i < width*linesToLoad; ++i)
							byteToColor(fileBuff[i], texBuff + i*3);
						glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, linesToLoad, 0, 0, width, height-linesToLoad);
						glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, linesToLoad, GL_RGB, GL_UNSIGNED_BYTE, texBuff);
                    	glDrawArrays(GL_TRIANGLES, 0, 6);
                    	SDL_GL_SwapWindow(win);
					} else {
						static int playedAlert = 0;
						if(!playedAlert) {
							playedAlert = 1;
    						printf("\aEnd of file.\n");
						}
					}
				}
			}
		}
	}
	
	//Let OS clean up.
	
	return 0;
}
