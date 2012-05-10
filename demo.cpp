
#include "demo.h"


void DemoInit(PP_Resource context, PPB_OpenGLES2* gl)
{
	//DBG_LOG("DemoInit");
}

void DemoRender(PP_Resource context, PPB_OpenGLES2* gl)
{
	gl->ClearColor(context, 1.0f, 0.0f, 0.0f, 1.0f);
	gl->Clear(context, GL_COLOR_BUFFER_BIT);

    SendInteger(10);

    //x++;
}

void DemoUpdate()
{
}

void DemoEnd()
{
}

