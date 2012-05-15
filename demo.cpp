
#include "demo.h"
#include "maths.h"

#include "textured_vs.h"
#include "textured_ps.h"

#include <cstdlib>
#include <cctype>
#include <memory.h>

GLuint uiDisplacementMapTexture;
static PP_Resource context;
static PPB_OpenGLES2* gl = 0;

GLuint vbo;
GLuint ibo;
GLuint GLProgram;

const int iDispMapWidth = 256;
const int iDispMapHeight = 256;

const uint32_t VA_POSITION_INDEX = 0; //VA=Vertex array
const uint32_t VA_TEXCOORD_INDEX = 1;

float Transform[4][4];

GLubyte* textureImage = 0;

GLuint CreateProgram(PP_Resource context, PPB_OpenGLES2* gl, const char* vs, const char* ps)
{
	GLuint shaders[2];
	GLuint program;
	GLint status = 0;

	//const char * allvs[] = {vs, 0};
	//const char * allps[] = {ps, 0};

	shaders[0] = gl->CreateShader(context, GL_VERTEX_SHADER);
	gl->ShaderSource(context, shaders[0], 1, &vs, 0);
	gl->CompileShader(context, shaders[0]);
	gl->GetShaderiv(context, shaders[0], GL_COMPILE_STATUS, &status);

	if(!status)
	{
		char log[256];
		DBG_LOG("Failed to vertex compile shader\n");

		gl->GetShaderInfoLog(context, shaders[1], 256, 0, log);

		DBG_LOG(log);
	}

	shaders[1] = gl->CreateShader(context, GL_FRAGMENT_SHADER);
	gl->ShaderSource(context, shaders[1], 1, &ps, 0);
	gl->CompileShader(context, shaders[1]);
	gl->GetShaderiv(context, shaders[1], GL_COMPILE_STATUS, &status);

	if(!status)
	{
		char log[256];
		DBG_LOG("Failed to pixel compile shader\n");

		gl->GetShaderInfoLog(context, shaders[1], 256, 0, log);

		DBG_LOG(log);
	}

	program = gl->CreateProgram(context);
	gl->AttachShader(context, program, shaders[0]);
	gl->AttachShader(context, program, shaders[1]);

	gl->BindAttribLocation(context, program, VA_POSITION_INDEX, "Position");
    gl->BindAttribLocation(context, program, VA_TEXCOORD_INDEX, "TexCoords0");
	
	gl->LinkProgram(context, program);
	gl->GetProgramiv(context, program, GL_LINK_STATUS, &status);

	if(!status)
	{
		DBG_LOG("Failed to link program\n");
	}

	return program;
}

void QuadInit(float width, float height, GLuint* puiQuadVBO, GLuint* puiQuadIBO)
{
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;

    const GLushort indices[] = {
        0, 1, 2, 3,
    };

    GLfloat vertices[8];

    const GLfloat texCoords[] = {
        0.0f,1.0f,
        1.0f,1.0f,
        0.0f,0.0f,
        1.0f, 0.0f,
    };

    vertices[0] = -halfWidth;
    vertices[1] = -halfHeight;
    vertices[2] = halfWidth;
    vertices[3] = -halfHeight;
    vertices[4] = -halfWidth;
    vertices[5] = halfHeight;
    vertices[6] = halfWidth;
    vertices[7] = halfHeight;

    gl->GenBuffers(context, 1, puiQuadVBO);
    gl->GenBuffers(context, 1, puiQuadIBO);

    gl->BindBuffer(context, GL_ARRAY_BUFFER, *puiQuadVBO);
    gl->BufferData(context, GL_ARRAY_BUFFER, sizeof(GLfloat)*16, NULL, GL_STATIC_DRAW);
    gl->BufferSubData(context, GL_ARRAY_BUFFER, 0, sizeof(GLfloat)*8, vertices);
    gl->BufferSubData(context, GL_ARRAY_BUFFER, sizeof(GLfloat)*8, sizeof(GLfloat)*8, texCoords);
    gl->BindBuffer(context, GL_ARRAY_BUFFER, 0);

    gl->BindBuffer(context, GL_ELEMENT_ARRAY_BUFFER, *puiQuadIBO);
    gl->BufferData(context, GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*4, indices, GL_STATIC_DRAW);
    gl->BindBuffer(context, GL_ELEMENT_ARRAY_BUFFER, 0);

}

void QuadDraw(GLuint uiQuadVBO, GLuint uiQuadIBO)
{
    gl->UseProgram(context, GLProgram);

    gl->UniformMatrix4fv(context, gl->GetUniformLocation(context, GLProgram, "Transform"), 1, GL_FALSE, (float*)&Transform[0][0]);
    gl->Uniform1i(context, gl->GetUniformLocation(context, GLProgram, "TextureBase"), 0);

    gl->BindTexture(context, GL_TEXTURE_2D, uiDisplacementMapTexture);

    gl->BindBuffer(context, GL_ARRAY_BUFFER, uiQuadVBO);
    gl->BindBuffer(context, GL_ELEMENT_ARRAY_BUFFER, uiQuadIBO);

    gl->EnableVertexAttribArray(context, VA_POSITION_INDEX);
    gl->EnableVertexAttribArray(context, VA_TEXCOORD_INDEX);

    gl->VertexAttribPointer(context, VA_POSITION_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
    gl->VertexAttribPointer(context, VA_TEXCOORD_INDEX, 2, GL_FLOAT, GL_FALSE, 0, (char*)(sizeof(GLfloat)*8));

    gl->DrawElements(context, GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);

}

void DemoInit(PP_Resource inContext, PPB_OpenGLES2* inGL)
{
    gl = inGL;
    context = inContext;

    GLProgram = CreateProgram(context, gl, psz_textured_vs, psz_textured_ps);

    gl->GenTextures(context, 1, &uiDisplacementMapTexture);

    if(!textureImage)
    {
        textureImage = (GLubyte*)malloc((iDispMapWidth*iDispMapHeight)*sizeof(GLubyte));

        memset(textureImage, 0, (iDispMapWidth*iDispMapHeight)*sizeof(GLubyte));
    }

    gl->BindTexture(context, GL_TEXTURE_2D, uiDisplacementMapTexture);

    gl->TexImage2D(context, GL_TEXTURE_2D, 0, GL_LUMINANCE, iDispMapWidth, iDispMapHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, textureImage);

    gl->TexParameteri(context, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->TexParameteri(context, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    QuadInit(1, 1, &vbo, &ibo);

    Identity(Transform);
}

void DemoRender(PP_Resource inContext, PPB_OpenGLES2* inGL)
{
	gl->ClearColor(context, 1.0f, 0.0f, 0.0f, 1.0f);
	gl->Clear(context, GL_COLOR_BUFFER_BIT);

    SendInteger(10);

    QuadDraw(vbo, ibo);

    //x++;
}

void DemoUpdate()
{
}

void DemoHandleString(const char* str, const uint32_t ui32StrLength)
{
    uint32_t i = 0;
    char* nextStr = (char*)str;

    //memset(textureImage, 0, (iDispMapWidth*iDispMapHeight)*sizeof(GLubyte));

    nextStr = strtok(nextStr, " ");
    while((nextStr != 0) && (i < ui32StrLength))
    {
        textureImage[i] = atoi(nextStr);
        nextStr = strtok(0, " ");

        ++i;
    }

    gl->BindTexture(context, GL_TEXTURE_2D, uiDisplacementMapTexture);
    gl->TexSubImage2D(context, GL_TEXTURE_2D, 0, 0, 0, iDispMapWidth, iDispMapHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, textureImage);
}

void DemoEnd()
{
    free(textureImage);
    textureImage = 0;
}

