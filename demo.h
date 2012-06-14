
#include "naclcontext.h"

#include "GLES2/gl2.h"

void DemoInit(NaCLContext* psNaCLContext, int width, int height);

void DemoRender(NaCLContext* psNaCLContext);

void DemoHandleKeyDown(uint32_t ui32KeyCode);
void DemoHandleKeyUp(uint32_t ui32KeyCode);

//Message passed from javascript to native client
void DemoHandleString(NaCLContext* psNaCLContext, const char* str, const uint32_t ui32StrLength);

void DemoEnd();
