/******************************************************************************

 @File         yuv2rgb.cpp

 @Title        Texturing

 @Version      

 @Copyright    Copyright (c) Imagination Technologies Limited.

 @Platform     Independant

 @Description  Shows how to use textures in OpenGL ES 2.0

******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <string>

#if defined(__APPLE__)
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#else
#if defined(__BADA__)
#include <FGraphicsOpengl2.h>

using namespace std;
using namespace Osp::Graphics::Opengl;
#else
#include <GLES2/gl2.h>
#endif
#endif

#include "PVRShell.h"
#include "yavtalib.h"

/******************************************************************************
 Defines
******************************************************************************/

// Index to bind the attributes to vertex shaders
#define VERTEX_ARRAY	0
#define TEXCOORD_ARRAY	1

// Size of the texture we create
#define TEX_SIZE		128

#define TWIDTH 640
#define THEIGHT 480
#define FTWIDTH 640.0
#define FTHEIGHT 480.0
static int gCount = 0;

/*!****************************************************************************
 Class implementing the PVRShell functions.
******************************************************************************/
class yuv2rgb : public PVRShell
{
	// The vertex and fragment shader OpenGL handles
	GLuint m_uiVertexShader, m_uiFragShader;

	// The program object containing the 2 shader objects
	GLuint m_uiProgramObject;

	// Texture handle
	GLuint	m_uiTexture;

	// VBO handle
	GLuint m_ui32Vbo;
	
	GLint m_ibaseMapLoc;
	GLint m_iTextureWidthLoc;
	GLint m_iTexelWidthLoc;

	//
	unsigned int m_ui32VertexStride;
	
	char* LoadShader( std::string filename );
	char* LoadYUV (std::string fileName, int *width, int *height );
	GLuint LoadTexture ( std::string fileName );
	
	struct device Device;
	void InitV4L( void );
	GLuint DequeueVideo( void );

public:
	virtual bool InitApplication();
	virtual bool InitView();
	virtual bool ReleaseView();
	virtual bool QuitApplication();
	virtual bool RenderScene();
};


/*!****************************************************************************
 @Function		InitApplication
 @Return		bool		true if no error occured
 @Description	Code in InitApplication() will be called by PVRShell once per
				run, before the rendering context is created.
				Used to initialize variables that are not dependant on it
				(e.g. external modules, loading meshes, etc.)
				If the rendering context is lost, InitApplication() will
				not be called again.
******************************************************************************/
bool yuv2rgb::InitApplication()
{
	struct device* dev = &Device;
	
	InitV4L();
	video_enable(dev, 1);

	return true;
}

/*!****************************************************************************
 @Function		QuitApplication
 @Return		bool		true if no error occured
 @Description	Code in QuitApplication() will be called by PVRShell once per
				run, just before exiting the program.
				If the rendering context is lost, QuitApplication() will
				not be called.
******************************************************************************/
bool yuv2rgb::QuitApplication()
{
	struct device* dev = &Device;
	video_enable(dev, 0);
	video_free_buffers(dev);
	video_close(&Device);
	return true;
}

char* yuv2rgb::LoadShader( std::string filename )
{
	char* buffer = NULL;
	FILE* pFile = NULL;
	long lSize = 0;

	pFile = fopen( filename.c_str(), "rb" );
	if(pFile == NULL)
		return NULL;

	fseek (pFile , 0 , SEEK_END);
	lSize = ftell (pFile);
	rewind (pFile);

	buffer = (char*)malloc( sizeof(char)*lSize );

	if(fread(buffer, 1, lSize, pFile) != (unsigned)lSize)
	{
		free(buffer);
		return NULL;
	}

	fclose(pFile);
	return buffer;
}

/*!****************************************************************************
 @Function		InitView
 @Return		bool		true if no error occured
 @Description	Code in InitView() will be called by PVRShell upon
				initialization or after a change in the rendering context.
				Used to initialize variables that are dependant on the rendering
				context (e.g. textures, vertex buffers, etc.)
******************************************************************************/
bool yuv2rgb::InitView()
{
	// Fragment and vertex shaders code
	const char* pszVertShader = "\
		attribute vec4 a_position;\
		attribute vec2 a_texCoord;\
		varying vec2 v_texCoord;\
		void main()\
		{\
			gl_Position = a_position;\
			v_texCoord = a_texCoord;\
		}";
		
	char* pszFragShader = LoadShader(std::string("yuv2rgb.frag"));

	// Create the fragment shader object
	m_uiFragShader = glCreateShader(GL_FRAGMENT_SHADER);
	//printf("%s(%d): WP - %d\n", __FUNCTION__, __LINE__, m_uiFragShader );

	// Load the source code into it
	glShaderSource(m_uiFragShader, 1, (const char**)&pszFragShader, NULL);

	// Compile the source code
	glCompileShader(m_uiFragShader);

	// Check if compilation succeeded
	GLint bShaderCompiled;
	glGetShaderiv(m_uiFragShader, GL_COMPILE_STATUS, &bShaderCompiled);
	if (!bShaderCompiled)
	{
		// An error happened, first retrieve the length of the log message
		int i32InfoLogLength, i32CharsWritten;
		glGetShaderiv(m_uiFragShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);

		// Allocate enough space for the message and retrieve it
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetShaderInfoLog(m_uiFragShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);

		/*
			Displays the message in a dialog box when the application quits
			using the shell PVRShellSet function with first parameter prefExitMessage.
		*/
		char* pszMsg = new char[i32InfoLogLength+256];
		strcpy(pszMsg, "Failed to compile fragment shader: ");
		strcat(pszMsg, pszInfoLog);
		PVRShellSet(prefExitMessage, pszMsg);

		delete [] pszMsg;
		delete [] pszInfoLog;
		return false;
	}

	// Loads the vertex shader in the same way
	m_uiVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(m_uiVertexShader, 1, (const char**)&pszVertShader, NULL);
	glCompileShader(m_uiVertexShader);
	glGetShaderiv(m_uiVertexShader, GL_COMPILE_STATUS, &bShaderCompiled);
	if (!bShaderCompiled)
	{
		int i32InfoLogLength, i32CharsWritten;
		glGetShaderiv(m_uiVertexShader, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetShaderInfoLog(m_uiVertexShader, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		char* pszMsg = new char[i32InfoLogLength+256];
		strcpy(pszMsg, "Failed to compile vertex shader: ");
		strcat(pszMsg, pszInfoLog);
		PVRShellSet(prefExitMessage, pszMsg);

		delete [] pszMsg;
		delete [] pszInfoLog;
		return false;
	}

	// Create the shader program
	m_uiProgramObject = glCreateProgram();

	// Attach the fragment and vertex shaders to it
	glAttachShader(m_uiProgramObject, m_uiFragShader);
	glAttachShader(m_uiProgramObject, m_uiVertexShader);

	// Bind the custom vertex attribute "myVertex" to location VERTEX_ARRAY
	glBindAttribLocation(m_uiProgramObject, VERTEX_ARRAY, "myVertex");
	// Bind the custom vertex attribute "myUV" to location TEXCOORD_ARRAY
	glBindAttribLocation(m_uiProgramObject, TEXCOORD_ARRAY, "myUV");

	// Link the program
	glLinkProgram(m_uiProgramObject);

	// Check if linking succeeded in the same way we checked for compilation success
	GLint bLinked;
	glGetProgramiv(m_uiProgramObject, GL_LINK_STATUS, &bLinked);

	if (!bLinked)
	{
		int i32InfoLogLength, i32CharsWritten;
		glGetProgramiv(m_uiProgramObject, GL_INFO_LOG_LENGTH, &i32InfoLogLength);
		char* pszInfoLog = new char[i32InfoLogLength];
		glGetProgramInfoLog(m_uiProgramObject, i32InfoLogLength, &i32CharsWritten, pszInfoLog);
		
		char* pszMsg = new char[i32InfoLogLength+256];
		strcpy(pszMsg, "Failed to link program: ");
		strcat(pszMsg, pszInfoLog);
		PVRShellSet(prefExitMessage, pszMsg);
		delete [] pszMsg;
		delete [] pszInfoLog;
		return false;
	}

	// Actually use the created program
	glUseProgram(m_uiProgramObject);

	// Sets the sampler2D variable to the first texture unit
	glUniform1i(glGetUniformLocation(m_uiProgramObject, "s_baseMap"), 0);
	glUniform1f(glGetUniformLocation(m_uiProgramObject, "texture_width"), FTWIDTH);
	glUniform1f(glGetUniformLocation(m_uiProgramObject, "texel_width"), 1.0/FTWIDTH);

	// Sets the clear color
	glClearColor(0.6f, 0.8f, 1.0f, 1.0f);

	return true;
}

/*!****************************************************************************
 @Function		ReleaseView
 @Return		bool		true if no error occured
 @Description	Code in ReleaseView() will be called by PVRShell when the
				application quits or before a change in the rendering context.
******************************************************************************/
bool yuv2rgb::ReleaseView()
{
	// Frees the texture
	glDeleteTextures(1, &m_uiTexture);

	// Release Vertex buffer object.
	glDeleteBuffers(1, &m_ui32Vbo);

	// Frees the OpenGL handles for the program and the 2 shaders
	glDeleteProgram(m_uiProgramObject);
	glDeleteShader(m_uiVertexShader);
	glDeleteShader(m_uiFragShader);
	return true;
}

char* yuv2rgb::LoadYUV ( std::string fileName, int *width, int *height )
{
	long lSize;
	char *buffer = NULL;
	FILE *f;

	f = fopen( fileName.c_str(), "rb" );
	if(f == NULL)
		return NULL;

	fseek (f , 0 , SEEK_END);
	lSize = ftell (f);
	rewind (f);

	buffer = (char*)malloc(lSize);
	if (buffer == NULL)
	{
		fclose(f);
		return NULL;
	}


	if(fread(buffer, 1, lSize, f) != (unsigned)lSize)
	{
		free(buffer);
		return NULL;
	}

	fclose(f);
	return buffer;
}

GLuint yuv2rgb::LoadTexture ( std::string fileName )
{
	GLuint texId;
	int width, height;
	char* buffer = (char*)LoadYUV ( fileName, &width, &height );

	width = TWIDTH;
	height = THEIGHT;

	if ( buffer == NULL )
	{
		printf( "Error loading (%s) image.\n", fileName.c_str() );
		return 0;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	

	glGenTextures ( 1, &texId );
	glBindTexture ( GL_TEXTURE_2D, texId );

	glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, buffer );
	//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT );
	//glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT );

	free ( buffer );

	return texId;
}

GLuint yuv2rgb::DequeueVideo( void )
{
	GLuint texId;
	int ret;
	struct v4l2_buffer buf;
	struct device* dev = &Device;
	enum buffer_fill_mode fill = BUFFER_FILL_NONE;
	
	//memset(&buf, 0, sizeof buf);
	buf.type = dev->type;
	buf.memory = dev->memtype;
	ret = ioctl(dev->fd, VIDIOC_DQBUF, &buf);

	//printf("%s: v4lbuf.sequence=%d\n", __FUNCTION__, buf.sequence );
	
	//if (dev->type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
		//video_verify_buffer(dev, buf.index);
	
	char* buffer = (char*)Device.buffers[buf.index].mem;

	gCount++;
	
	glPixelStorei(GL_UNPACK_ALIGNMENT,4);

	glGenTextures ( 1, &texId );
	glBindTexture ( GL_TEXTURE_2D, texId );

	glTexImage2D ( GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, TWIDTH, THEIGHT, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, buffer );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	
	video_queue_buffer(dev, buf.index, fill);

	return texId;
}

/*!****************************************************************************
 @Function		RenderScene
 @Return		bool		true if no error occured
 @Description	Main rendering loop function of the program. The shell will
				call this function every frame.
				eglSwapBuffers() will be performed by PVRShell automatically.
				PVRShell will also manage important OS events.
				Will also manage relevent OS events. The user has access to
				these events through an abstraction layer provided by PVRShell.
******************************************************************************/
bool yuv2rgb::RenderScene()
{
#if 0
	GLuint baseMapTexId = LoadTexture ( std::string("yuv_640x480_1.raw") );
#else
	GLuint baseMapTexId = DequeueVideo();
#endif

	if ( baseMapTexId == 0 )
		return false;

	GLfloat vVertices[] = { -1.0f,  1.0f, 0.0f,  // Position 0
				0.0f,  0.0f,        // TexCoord 0 
				-1.0f, -1.0f, 0.0f,  // Position 1
				0.0f,  1.0f,        // TexCoord 1
				1.0f, -1.0f, 0.0f,  // Position 2
				1.0f,  1.0f,        // TexCoord 2
				1.0f,  1.0f, 0.0f,  // Position 3
				1.0f,  0.0f         // TexCoord 3
				};
	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

	// Set the viewport
	glViewport ( 0, 0, TWIDTH, THEIGHT );

	// Clear the color buffer
	glClear ( GL_COLOR_BUFFER_BIT );

	GLint positionLoc = glGetAttribLocation ( m_uiProgramObject, "a_position" );
	GLint texCoordLoc = glGetAttribLocation ( m_uiProgramObject, "a_texCoord" );

	// Load the vertex position
	glVertexAttribPointer ( positionLoc, 3, GL_FLOAT, 
				GL_FALSE, 5 * sizeof(GLfloat), vVertices );
	// Load the texture coordinate
	glVertexAttribPointer ( texCoordLoc, 2, GL_FLOAT,
				GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3] );

	glEnableVertexAttribArray ( positionLoc );
	glEnableVertexAttribArray ( texCoordLoc );

	// Bind the base map
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, baseMapTexId );

	glDrawElements ( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices );

	//usleep(1000*100);
	//printf("WP: userData->baseMapTexId=0x%x\n", userData->baseMapTexId);

	glDeleteTextures ( 1, &baseMapTexId );
	
	return true;
}
//Lynx
void yuv2rgb::InitV4L( void )
{
        video_open(&Device, "/dev/video6", 0);
        Device.memtype = V4L2_MEMORY_MMAP;

	video_enum_formats(&Device, V4L2_BUF_TYPE_VIDEO_CAPTURE);
	video_enum_formats(&Device, V4L2_BUF_TYPE_VIDEO_OUTPUT);
	video_enum_formats(&Device, V4L2_BUF_TYPE_VIDEO_OVERLAY);

	video_get_format(&Device);

	video_prepare_capture(&Device, V4L_BUFFERS_DEFAULT, 0, NULL, BUFFER_FILL_NONE);
}

/*!****************************************************************************
 @Function		NewDemo
 @Return		PVRShell*		The demo supplied by the user
 @Description	This function must be implemented by the user of the shell.
				The user should return its PVRShell object defining the
				behaviour of the application.
******************************************************************************/
PVRShell* NewDemo()
{
	return new yuv2rgb();
}

/******************************************************************************
 End of file (yuv2rgb.cpp)
******************************************************************************/

