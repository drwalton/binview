#include <GL/glew.h>
#include <SDL.h>
#ifdef WIN32
#undef main
#define _WIN32_LEAN_AND_MEAN	
#include <Windows.h>
#else
#include <sys/stat.h>
#endif
#include <stdio.h>

#ifdef __APPLE__
#define USE_OGL_4_1
#endif

//General
int width = 500, height = 800;
const long maxScrollLines = 40, scrollSpeed = 8;
GLbyte *fileBuff, *texBuff;
GLbyte *fileChunk = NULL, *texChunk = NULL;
FILE *file;
char winTitle[30];
long fileSize, fileSizeKb;

//OpenGL
GLuint vertBuffer, texCoordBuffer, vao;
GLuint vertShader, fragShader, program;
GLuint texture;
const float verts[] = {
	-1.f, 1.f, -1.f, -1.f, 1.f, 1.f, 1.f, 1.f, -1.f, -1.f, 1.f, -1.f };
const float texCoords[] = {
	0.f, 1.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f };

//SDL
SDL_Window *win;
SDL_GLContext context;
SDL_Renderer *renderer;


const GLbyte BLACK[] = {0, 0, 0, 0};
void byteToColor(GLbyte byte, GLbyte *color) {
	if(byte == 0) memcpy(color, BLACK, 4);
	else if (byte < 127) {
		color[0] = 0;
		color[1] = 0;
		color[2] = byte + 128;
		color[3] = 0;
	} else {
		color[0] = byte;
		color[1] = 0;
		color[2] = 0;
		color[3] = 0;
	}
}

const GLchar *const vertShaderSource =
#ifdef USE_OGL_4_1
"#version 410\n"
#else
"#version 130\n"
#endif
"\n"
"in vec2 vPos;\n"
"in vec2 vTex;\n"
"\n"
"out vec2 tex;\n"
"\n"
"void main() {\n"
"	tex = vTex;\n"
"	gl_Position = vec4(vPos, 0, 1);\n"
"}\n"
;
const GLchar *const fragShaderSource =
#ifdef USE_OGL_4_1
"#version 410\n"
#else
"#version 130\n"
#endif
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

long lminl(long a, long b) {
	return a < b ? a : b;
}
void setupSDLAndGLEW()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		printf("ERROR: unable to start SDL\n");
		exit(-1); //unable to init SDL.
	}
	
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	#ifdef USE_OGL_4_1
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	#else 
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	#endif
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	
	Uint32 winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	
	win = SDL_CreateWindow("binview", 30, 30,
		width, height, winFlags);
	context = SDL_GL_CreateContext(win);
	renderer = SDL_CreateRenderer(win, -1, 0);
	SDL_GL_MakeCurrent(win, context);
	
	glewExperimental = GL_TRUE;
	{
    	GLenum r = glewInit();
    	if (r != GLEW_OK) {
			printf("ERROR: Unable to start glew\n");
    		exit(-2); //unable to init glew.
    	}
	}
	if (!glewIsSupported("GL_VERSION_3_0")) {
		printf("ERROR: OpenGL 3.0 is not supported");
		exit(-3); //opengl_3_0 not supported.
	}
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
void setupVertexBuffers()
{
	GLint
		vertAttrib = glGetAttribLocation(program, "vPos"),
		texAttrib = glGetAttribLocation(program, "vTex");
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertBuffer);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(float), verts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(vertAttrib);
	glVertexAttribPointer(vertAttrib, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glGenBuffers(1, &texCoordBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBuffer);
	glBufferData(GL_ARRAY_BUFFER, 12*sizeof(float), texCoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 0, NULL);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}
void makeShaderProgram()
{
	vertShader = glCreateShader(GL_VERTEX_SHADER);
	fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertShader, 1, &vertShaderSource, 0);
	glShaderSource(fragShader, 1, &fragShaderSource, 0);
	if(!compileAndCheckShader(vertShader) ||
	   !compileAndCheckShader(fragShader)) {
		printf("ERROR: Unable to compile shaders\n");
		exit(-4); //Shaders failed to compile.
	}
	program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);
	if(!linkAndCheckProgram(program)) {
		printf("ERROR: Unable to link shaders\n");
		exit(-5); //Shaders failed to link.
	}
}
void fillTexture(GLboolean textureSizeChanged)
{
	if (textureSizeChanged) {
		if(fileChunk) free(fileChunk);
		if(texChunk) free(texChunk);
		fileChunk = malloc(width*height);
		texChunk = malloc(width*height * 4);
	}
	fread(fileChunk, 1, width*height, file);
	for(int r = 0; r < height; ++r) {
		for(int c = 0; c < width; ++c) {
			byteToColor(fileChunk[(height-r-1)*width + c], texChunk + (r*width + c)*4);
		}
	}
	glBindTexture(GL_TEXTURE_2D, texture);
	if (textureSizeChanged) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
			width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texChunk);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	} else {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
			width, height, GL_RGBA, GL_UNSIGNED_BYTE, texChunk);

	}
}
void scrollDown(long scrollAmt)
{
	long linesToLoad = lminl(-scrollAmt, maxScrollLines);
	//Check we aren't at the end of the file.
	if(!feof(file)) {
		fread(fileBuff, 1, width*linesToLoad, file);
		for(size_t r = 0; r < linesToLoad; ++r) {
			for(size_t c = 0; c < width; ++c) {
				byteToColor(
					fileBuff[(linesToLoad-r-1)*width + c],
					texBuff + (r*width + c)*4);
			}
		}
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, linesToLoad, 0, 0, width, height-linesToLoad);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, linesToLoad, GL_RGBA, GL_UNSIGNED_BYTE, texBuff);
    	glDrawArrays(GL_TRIANGLES, 0, 6);
    	SDL_GL_SwapWindow(win);
	} else {
		//At end of file.
		//Play alert the first time the end of the file is reached.
		static int playedAlert = 0;
		if(!playedAlert) {
			playedAlert = 1;
			printf("\aEnd of file.\n");
		}
	}
}
void scrollUp(long scrollAmt)
{
    //First, make sure position is multiple of width.
    //This might not be the case if we just reached the end of the file.
    long currPos = ftell(file);

    if(currPos > height*width) { //Check if we are able to scroll up.
		if (currPos % width != 0) {
			currPos -= currPos % width;
			fseek(file, currPos, SEEK_SET);
		}
    	long linesToLoad = lminl(scrollAmt, maxScrollLines);
    	linesToLoad = lminl((currPos - (height*width)) / width, linesToLoad);
    	if(linesToLoad <= 0) {
    		printf("ERROR: negative number of lines to load; "
				"should not reach this point.\n");
    		exit(-6);
    	}
    	//Move back by number of lines + screen height
    	fseek(file, -width*(linesToLoad + height), SEEK_CUR);
    	//Load data from file.
    	fread(fileBuff, 1, width*linesToLoad, file);
    	//Scroll back to previous position.
    	fseek(file, width*(height-linesToLoad), SEEK_CUR);
    	//DrascrollAmtw
    	for(size_t r = 0; r < linesToLoad; ++r) {
    		for(size_t c = 0; c < width; ++c) {
    			byteToColor(
    				fileBuff[(linesToLoad-r-1)*width + c],
    				texBuff + (r*width + c)*4);

    		}
    	}
    	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, linesToLoad, width, height-linesToLoad);
    	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, height-linesToLoad, width, linesToLoad, GL_RGBA, GL_UNSIGNED_BYTE, texBuff);
    	glDrawArrays(GL_TRIANGLES, 0, 6);
    	SDL_GL_SwapWindow(win);
	}
}
void resizeWindow(int newWidth, int newHeight)
{
	long pos = ftell(file);
	//Back up to start of window.
	if (pos > width*height) {
		pos -= width*height;
		if (pos >= pos % newWidth) pos -= pos % newWidth; //Ensure pos is a multiple of a line width.
	} else {
		pos = 0;
	}
	
	fseek(file, pos, SEEK_SET);
	width = newWidth;
	height = newHeight;
	//Regenerate texture.
	fillTexture(GL_TRUE);
	glViewport(0, 0, width, height);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	SDL_GL_SwapWindow(win);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	SDL_GL_SwapWindow(win);
	free(fileBuff);
	free(texBuff);
	fileBuff = texBuff = NULL;
	fileBuff = malloc(maxScrollLines*width);
	texBuff = malloc(maxScrollLines*width*4);
}

long getFileSize(const char *filename) {
	long size;
#ifdef _WIN32
	LARGE_INTEGER fileSize;
	HANDLE file = CreateFile(TEXT(filename), GENERIC_READ, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		printf("ERROR: unable to open file.\n");
		exit(-1);
	}
	if (!GetFileSizeEx(file, &fileSize)) {
		printf("ERROR: unable to obtain size of file.\n");
		exit(-1);
	}
	CloseHandle(file);
	size = fileSize.QuadPart;
#else
	struct stat st;

	if (stat(filename, &st) != 0) {
		printf("ERROR: unable to obtain size of file.\n");
		exit(-1);
	}
	size = st.st_size;
#endif
	return size;
}

int main(int argc, char *argv[])
{
	if(argc < 2) {
		printf("Usage: binview [filename] <width> <height>\n");
		return -1; //No filename supplied.
	}
	printf("Loading file: %s\n", argv[1]);
	if(argc >= 4) {
		width = atoi(argv[2]);
		height = atoi(argv[3]);
	}

	//Get size of file.
	fileSize = getFileSize(argv[1]);
	fileSizeKb = fileSize / 1000;

	file = fopen(argv[1], "rb");
	if (!file) {
		printf("Unable to open file.\n");
		return -7;
	}
	
	setupSDLAndGLEW();
	
	//Make texture, and populate with initial chunk of file.
	glGenTextures(1, &texture);
	fillTexture(GL_TRUE);

	//Create shaders.
	makeShaderProgram();
	
	//Make vertex buffers.
	setupVertexBuffers();
	
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
	
	fileBuff = malloc(maxScrollLines*width);
	texBuff = malloc(maxScrollLines*width*4);
	glReadBuffer(GL_FRONT);
	
	long scrollAmt;
	while (running) {
		SDL_Delay(30);
		scrollAmt = 0;
		while (SDL_PollEvent(&event)) {
			if (eventIsQuit(event)) {
				running = GL_FALSE;
			}
			if (event.type == SDL_MOUSEWHEEL) {
				scrollAmt += event.wheel.y;
			}
			else if(event.type == SDL_WINDOWEVENT) {
				if(event.window.event == SDL_WINDOWEVENT_RESIZED) {
    				int newWidth = event.window.data1;
    				int newHeight = event.window.data2;
					resizeWindow(newWidth, newHeight);
				}
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_PAGEDOWN) {
					if (!feof(file)) {
						fillTexture(GL_FALSE);
						glDrawArrays(GL_TRIANGLES, 0, 6);
						SDL_GL_SwapWindow(win);
					}
				}
				else if (event.key.keysym.sym == SDLK_PAGEUP) {
					long pos = ftell(file);
					if (pos != 0) {
						if (pos < width*height*2) {
							pos = 0;
						} else {
							pos -= width*height*2;
						}
						fseek(file, pos, SEEK_SET);
						fillTexture(GL_FALSE);
						glDrawArrays(GL_TRIANGLES, 0, 6);
						SDL_GL_SwapWindow(win);

					}

				}
			}
		}
		scrollAmt *= scrollSpeed;
		if(scrollAmt > 0) {
			//Scrolled up.
			scrollUp(scrollAmt);
		} else if(scrollAmt < 0) {
			//Scrolled down.
			scrollDown(scrollAmt);
		}
		long p = ftell(file);
		p /= 1000;
		sprintf(winTitle, "binview: %ldKB of %ldKB", p, fileSizeKb);
		SDL_SetWindowTitle(win, winTitle);
	}
	
	//Let OS clean up.
	
	return 0;
}
