#ifndef NACL_CONTEXT_H
#define NACL_CONTEXT_H

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_graphics_3d.h"

#include "ppapi/c/ppp.h"
#include "ppapi/c/ppp_instance.h"
#include "ppapi/c/ppp_messaging.h"
#include "ppapi/c/ppp_input_event.h"

#include "ppapi/c/ppb.h"
#include "ppapi/c/ppb_instance.h"
#include "ppapi/c/ppb_messaging.h"
#include "ppapi/c/ppb_var.h"
#include "ppapi/c/ppb_input_event.h"
#include "ppapi/c/ppb_core.h"
#include "ppapi/c/ppb_view.h"

#include "ppapi/c/ppb_graphics_3d.h"
#include "ppapi/c/ppp_graphics_3d.h"
#include "ppapi/c/ppb_url_request_info.h"
#include "ppapi/c/ppb_url_loader.h"

#include "ppapi/gles2/gl2ext_ppapi.h"

typedef struct NaCLContext_TAG
{
    int32_t i32PluginWidth;
    int32_t i32PluginHeight;

    //Handles
    PP_Resource hRenderContext;
    PP_Instance hModule;

    //Interfaces to pepper systems.
    PPB_Messaging* psMessagingInterface;
    PPB_Var* psVarInterface;
    PPB_OpenGLES2* psGL;
    PPB_Graphics3D* psG3D;
    PPB_Instance* psInstanceInterface;
    PPB_View* psView;
    PPB_Core* psCore;
    PPB_Var* psVar;
    PPB_InputEvent* psInputEventInterface;
    PPB_KeyboardInputEvent* psKeyboard;
    PPB_URLRequestInfo* psURLRequest;
    PPB_URLLoader* psURLLoader;
} NaCLContext;

#define DEBUG_MESSAGES

#define DBG_LOG_PREFIX "DBG: "

#ifdef DEBUG_MESSAGES
	#define DBG_LOG(MSG) SendString(MSG);
#else
	#define DBG_LOG(MSG)
#endif

extern void SendString(const char* str);
extern void SendInteger(int val);

#endif

