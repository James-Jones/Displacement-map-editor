
#include "ppapi/gles2/gl2ext_ppapi.h"

#include "GLES2/gl2.h"

#define DEBUG_MESSAGES

#define DBG_LOG_PREFIX "DBG: "

#ifdef DEBUG_MESSAGES
	#define DBG_LOG(MSG) SendString(MSG);
#else
	#define DBG_LOG(MSG)
#endif

extern void SendString(const char* str);
extern void SendInteger(int val);

void DemoInit(PP_Resource context, PPB_OpenGLES2* gl, int width, int height);

void DemoRender(PP_Resource context, PPB_OpenGLES2* gl);

void DemoUpdate();

void DemoHandleKeyDown(uint32_t ui32KeyCode);
void DemoHandleKeyUp(uint32_t ui32KeyCode);

//Message passed from javascript to native client
void DemoHandleString(const char* str, const uint32_t ui32StrLength);

void DemoEnd();
