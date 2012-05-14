
#include "demo.h"

#include <cstdlib>
#include <cctype>

GLuint uiDisplacementMapTexture;
static PP_Resource context;
static PPB_OpenGLES2* gl = 0;

const int iDispMapWidth = 512;
const int iDispMapHeight = 512;

void DemoInit(PP_Resource inContext, PPB_OpenGLES2* inGL)
{
    gl = inGL;
    context = inContext;
	//DBG_LOG("DemoInit");
    gl->GenTextures(context, 1, &uiDisplacementMapTexture);

    gl->BindTexture(context, GL_TEXTURE_2D, uiDisplacementMapTexture);
    gl->TexImage2D(context, GL_TEXTURE_2D, 0, GL_LUMINANCE, iDispMapWidth, iDispMapHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
}

void DemoRender(PP_Resource inContext, PPB_OpenGLES2* inGL)
{
	gl->ClearColor(context, 1.0f, 0.0f, 0.0f, 1.0f);
	gl->Clear(context, GL_COLOR_BUFFER_BIT);

    SendInteger(10);

    //x++;
}

void DemoUpdate()
{
}

void DemoHandleString(const char* str, const uint32_t ui32StrLength)
{
    uint32_t i = 0;
    const char* nextStr = str;

    GLubyte* data = (GLubyte*)malloc((iDispMapWidth*iDispMapHeight)*sizeof(GLubyte));

    while(i < (iDispMapWidth*iDispMapHeight))
    {
        if(nextStr-str<ui32StrLength)
        {
            break;
        }

        //Convert to integer
        data[i] = atoi(nextStr);


        //Next number in the space-delimited sequence
        while(!isspace(nextStr[0]) && (nextStr-str<ui32StrLength))
        {
            nextStr++;
        }

        ++i;
    }

    gl->BindTexture(context, GL_TEXTURE_2D, uiDisplacementMapTexture);
    gl->TexImage2D(context, GL_TEXTURE_2D, 0, 0, 0, iDispMapWidth, iDispMapHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);

    free(data);
}

void DemoEnd()
{
}

