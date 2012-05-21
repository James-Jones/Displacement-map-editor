#include "demo.h"
#include "maths.h"
#include "cJSON.h"

#include "textured_vs.h"
#include "textured_ps.h"
//#include "cube_json.h"

#include "cube_json.h"

#include "jpgd.h"

#include "checkerboard_jpg.h"

#include <cstdlib>
#include <cctype>
#include <memory.h>
#include <cstdio>
#include <sys/time.h>

#define max(a, b) ((a > b) ? (a) : (b))

//Texture
GLuint uiDisplacementMapTexture;
GLubyte* textureImage = 0;
const int iDispMapWidth = 256;
const int iDispMapHeight = 256;

GLuint uiMeshBaseTexture;

//GL interface
static PP_Resource context;
static PPB_OpenGLES2* gl = 0;

//Linked shaders
GLuint GLProgram;

//Vertex input locations
const uint32_t VA_POSITION_INDEX = 0; //VA=Vertex array
const uint32_t VA_TEXCOORD_INDEX = 1;
const uint32_t VA_NORMAL_INDEX = 2;

//Timer
timeval startTime;
uint64_t ui64BaseTimeMS;

//Transform
float angle;
typedef struct
{
    float projection[4][4];
    float camera[4][4];
    float rot[4][4];
    float scale[4][4];
    float xform[4][4];
} Transformations;

//Mesh vertices, indices and MVP.
GLuint jsonVBO;
GLuint jsonIBO;
int jsonIdxCount;
int jsonVtxCount;
Transformations jsonMeshTransform;


uint64_t GetElapsedTimeMS()
{
    timeval t;
    gettimeofday(&t, NULL);

    uint64_t secs  = t.tv_sec - startTime.tv_sec;
    uint64_t uSecs = t.tv_usec;

    // Make granularity 1 ms
    return (secs * 1000) + (uSecs / 1000);
}

GLuint CreateProgram(PP_Resource context, PPB_OpenGLES2* gl, const char* vs, const char* ps)
{
	GLuint shaders[2];
	GLuint program;
	GLint status = 0;

	shaders[0] = gl->CreateShader(context, GL_VERTEX_SHADER);
	gl->ShaderSource(context, shaders[0], 1, &vs, 0);
	gl->CompileShader(context, shaders[0]);
	gl->GetShaderiv(context, shaders[0], GL_COMPILE_STATUS, &status);

	if(!status)
	{
		char log[256];
		DBG_LOG("Failed to vertex compile shader\n");

		gl->GetShaderInfoLog(context, shaders[0], 256, 0, log);

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
    gl->BindAttribLocation(context, program, VA_NORMAL_INDEX, "Normal");
	
	gl->LinkProgram(context, program);
	gl->GetProgramiv(context, program, GL_LINK_STATUS, &status);

	if(!status)
	{
		DBG_LOG("Failed to link program\n");
	}

	return program;
}

void LoadJSONMesh(const char* mesh, Transformations* psMatrices, int* piNumIndices, int* piNumVertices, GLuint* puiVBO, GLuint* puiIBO)
{
    uint16_t* pui16Indices;
    float* pfVertices;
    float* pfTexCoords;
    float* pfNormals;
    int idx;
    int vtx;
    int iNumIndices;
    int iNumVertices;
    float fMinX;
    float fMinY;
    float fMinZ;
    float fMaxX;
    float fMaxY;
    float fMaxZ;
    float fCenterX;
    float fCenterY;
    float fCenterZ;
    cJSON *psIndices;
    cJSON *psVertices;
    cJSON *psTexCoords;
    cJSON *psNormals;
    cJSON* psBoundingBox;
    float fScale;

    cJSON *psRoot = cJSON_Parse(mesh);

    if(!psRoot)
    {
        DBG_LOG("No root JSON");
        return;
    }

    psIndices = cJSON_GetObjectItem(psRoot,"idx");

    if(!psIndices)
    {
        DBG_LOG("No indices JSON");
        cJSON_Delete(psRoot);
        return;
    }

    psVertices = cJSON_GetObjectItem(psRoot,"vtxpos");
    if(!psVertices)
    {
        DBG_LOG("No vertices JSON");
        cJSON_Delete(psRoot);
        return;
    }

    psTexCoords = cJSON_GetObjectItem(psRoot,"texcoord");
    if(!psTexCoords)
    {
        DBG_LOG("No texcoords JSON");
        cJSON_Delete(psRoot);
        return;
    }
    
    psNormals = cJSON_GetObjectItem(psRoot,"normal");
    if(!psNormals)
    {
        DBG_LOG("No normals JSON");
        cJSON_Delete(psRoot);
        return;
    }

    psBoundingBox = cJSON_GetObjectItem(psRoot, "bndbox");
    if(!psBoundingBox)
    {
        DBG_LOG("No no bndbox JSON");
        cJSON_Delete(psRoot);
        return;
    }

    iNumIndices = cJSON_GetArraySize(psIndices);

    pui16Indices = (uint16_t*)malloc(sizeof(uint16_t) * iNumIndices);

    for(idx = 0; idx < iNumIndices; ++idx)
    {
        cJSON* psIndex = cJSON_GetArrayItem(psIndices, idx);
        pui16Indices[idx] = psIndex->valueint;
    }

    iNumVertices = cJSON_GetArraySize(psVertices) / 3; //GetArraySize will give the number of floats in vertex position array. Always XYZ.

    pfVertices = (float*)malloc(sizeof(float) * iNumVertices * 3);
    pfTexCoords = (float*)malloc(sizeof(float) * iNumVertices * 3);
    pfNormals = (float*)malloc(sizeof(float) * iNumVertices * 3);
    for(vtx = 0; vtx < (iNumVertices * 3);)
    {
        cJSON* psVertex;
        cJSON* psTexC;
        cJSON* psNrm;

        psVertex = cJSON_GetArrayItem(psVertices, vtx);
        pfVertices[vtx] = psVertex->valuedouble;

        psTexC = cJSON_GetArrayItem(psTexCoords, vtx);
        pfTexCoords[vtx] = psTexC->valuedouble;

        psNrm = cJSON_GetArrayItem(psNormals, vtx);
        pfNormals[vtx] = psNrm->valuedouble;

        ++vtx;

        psVertex = cJSON_GetArrayItem(psVertices, vtx);
        pfVertices[vtx] = psVertex->valuedouble;

        psTexC = cJSON_GetArrayItem(psTexCoords, vtx);
        pfTexCoords[vtx] = psTexC->valuedouble;

        psNrm = cJSON_GetArrayItem(psNormals, vtx);
        pfNormals[vtx] = psNrm->valuedouble;

        ++vtx;

        psVertex = cJSON_GetArrayItem(psVertices, vtx);
        pfVertices[vtx] = psVertex->valuedouble;

        psTexC = cJSON_GetArrayItem(psTexCoords, vtx);
        pfTexCoords[vtx] = psTexC->valuedouble;

        psNrm = cJSON_GetArrayItem(psNormals, vtx);
        pfNormals[vtx] = psNrm->valuedouble;

        ++vtx;
    }

    fMinX = cJSON_GetArrayItem(psBoundingBox, 0)->valuedouble;
    fMinY = cJSON_GetArrayItem(psBoundingBox, 1)->valuedouble;
    fMinZ = cJSON_GetArrayItem(psBoundingBox, 2)->valuedouble;
    fMaxX = cJSON_GetArrayItem(psBoundingBox, 3)->valuedouble;
    fMaxY = cJSON_GetArrayItem(psBoundingBox, 4)->valuedouble;
    fMaxZ = cJSON_GetArrayItem(psBoundingBox, 5)->valuedouble;

    cJSON_Delete(psRoot);

    //Translation and scaling to get the object to the middle of the screen.

    Identity(psMatrices->rot);
    Identity(psMatrices->scale);
    Identity(psMatrices->xform);

	fScale = fMaxX-fMinX;
	fScale = max(fMaxY - fMinY,fScale);
	fScale = max(fMaxZ - fMinZ,fScale);
	fScale = 1.f / fScale;
    Scale(psMatrices->scale, fScale, fScale, fScale);

    fCenterX = (fMinX + fMaxX) / 2.0f;
    fCenterY = (fMinY + fMaxY) / 2.0f;
    fCenterZ = (fMinZ + fMaxZ) / 2.0f;
    Translate(psMatrices->xform, -fCenterX, -fCenterY, -fCenterZ );

    //Put the vertices and indices into device memory.

    gl->GenBuffers(context, 1, puiVBO);
    gl->GenBuffers(context, 1, puiIBO);

    gl->BindBuffer(context, GL_ARRAY_BUFFER, *puiVBO);
    gl->BufferData(context, GL_ARRAY_BUFFER, sizeof(GLfloat)*(iNumVertices*9), NULL, GL_STATIC_DRAW);
    gl->BufferSubData(context, GL_ARRAY_BUFFER, 0, sizeof(GLfloat)*(iNumVertices*3), pfVertices);
    gl->BufferSubData(context, GL_ARRAY_BUFFER, sizeof(GLfloat)*(iNumVertices*3), sizeof(GLfloat)*(iNumVertices*3), pfTexCoords);
    gl->BufferSubData(context, GL_ARRAY_BUFFER, sizeof(GLfloat)*(iNumVertices*6), sizeof(GLfloat)*(iNumVertices*3), pfNormals);
    gl->BindBuffer(context, GL_ARRAY_BUFFER, 0);

    gl->BindBuffer(context, GL_ELEMENT_ARRAY_BUFFER, *puiIBO);
    gl->BufferData(context, GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*iNumIndices, pui16Indices, GL_STATIC_DRAW);
    gl->BindBuffer(context, GL_ELEMENT_ARRAY_BUFFER, 0);

    free(pui16Indices);
    free(pfVertices);
    free(pfTexCoords);

    *piNumIndices = iNumIndices;
    *piNumVertices = iNumVertices;
}

void DrawJSONMesh(Transformations* psMatrices, int iNumVertices, int iNumIndices, GLuint uiVBO, GLuint uiIBO)
{
    float afTransform[4][4];//The final matrix
    float model[4][4];
    float view[4][4];
    float modelview[4][4];

    gl->UseProgram(context, GLProgram);

    Identity(afTransform);

    Identity(view);
    MultMatrix(view, view, psMatrices->camera);
    
    Identity(model);
    MultMatrix(model, model, psMatrices->scale);
    MultMatrix(model, model, psMatrices->rot);
    MultMatrix(model, model, psMatrices->xform);

    MultMatrix(modelview, model, view);

	MultMatrix(afTransform,  modelview, psMatrices->projection);

    gl->UniformMatrix4fv(context, gl->GetUniformLocation(context, GLProgram, "Transform"), 1, GL_FALSE, (float*)&afTransform[0][0]);
    gl->Uniform1i(context, gl->GetUniformLocation(context, GLProgram, "TextureBase"), 0);

    gl->Uniform1i(context, gl->GetUniformLocation(context, GLProgram, "DispMap"), 1);

    gl->Uniform1f(context, gl->GetUniformLocation(context, GLProgram, "DISPLACEMENT_COEFFICIENT"), 2.0f);

    gl->ActiveTexture(context, GL_TEXTURE0+1);
    gl->BindTexture(context, GL_TEXTURE_2D, uiDisplacementMapTexture);
    gl->ActiveTexture(context, GL_TEXTURE0);
    gl->BindTexture(context, GL_TEXTURE_2D, uiMeshBaseTexture);

    gl->BindBuffer(context, GL_ARRAY_BUFFER, uiVBO);
    gl->BindBuffer(context, GL_ELEMENT_ARRAY_BUFFER, uiIBO);

    gl->EnableVertexAttribArray(context, VA_POSITION_INDEX);
    gl->EnableVertexAttribArray(context, VA_TEXCOORD_INDEX);
    gl->EnableVertexAttribArray(context, VA_NORMAL_INDEX);

    gl->VertexAttribPointer(context, VA_POSITION_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
    gl->VertexAttribPointer(context, VA_TEXCOORD_INDEX, 3, GL_FLOAT, GL_FALSE, 0, (char*)(sizeof(GLfloat)*(iNumVertices*3)));
    gl->VertexAttribPointer(context, VA_NORMAL_INDEX, 3, GL_FLOAT, GL_FALSE, 0, (char*)(sizeof(GLfloat)*(iNumVertices*6)));

    gl->DrawElements(context, GL_TRIANGLES, iNumIndices, GL_UNSIGNED_SHORT, 0);
}


void DemoInit(PP_Resource inContext, PPB_OpenGLES2* inGL, int width, int height)
{
    const float aspectRatio = (float)width / (float)height;
    const float fieldOfView = 45.0f;

        unsigned char* jpeg = 0;
        int jpegWidth = 0;
        int jpegHeight = 0;
        int iNumComponents = 0;
        GLenum eFormat;

    gl = inGL;
    context = inContext;

    Identity(jsonMeshTransform.projection);
    Persp(jsonMeshTransform.projection, fieldOfView, aspectRatio,
        1.0, 1000.0);

    if(!textureImage) //First time init has been called
    {
        textureImage = (GLubyte*)malloc((iDispMapWidth*iDispMapHeight)*sizeof(GLubyte));

        memset(textureImage, 0, (iDispMapWidth*iDispMapHeight)*sizeof(GLubyte));

        gettimeofday(&startTime, NULL);

        ui64BaseTimeMS = GetElapsedTimeMS();
        angle = 0;

    }

        GLProgram = CreateProgram(context, gl, psz_textured_vs, psz_textured_ps);

        gl->GenTextures(context, 1, &uiDisplacementMapTexture);

        gl->BindTexture(context, GL_TEXTURE_2D, uiDisplacementMapTexture);

        gl->TexImage2D(context, GL_TEXTURE_2D, 0, GL_LUMINANCE, iDispMapWidth, iDispMapHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, textureImage);

        gl->TexParameteri(context, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->TexParameteri(context, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);



        gl->GenTextures(context, 1, &uiMeshBaseTexture);

        gl->BindTexture(context, GL_TEXTURE_2D, uiMeshBaseTexture);

// Loads a JPEG image from a memory buffer.
  // req_comps can be 1 (grayscale), 3 (RGB), or 4 (RGBA).
  // On return, width/height will be set to the image's dimensions, and actual_comps will be set 
  // to either 1 (grayscale) or 3 (RGB).
        jpeg = jpgd::decompress_jpeg_image_from_memory(psz_checkerboard_jpg, sizeof(psz_checkerboard_jpg), 
                                       &jpegWidth, &jpegHeight, &iNumComponents, 3);

        if(!jpeg)
        {
            DBG_LOG("Bad jpeg texture");
        }

        switch(iNumComponents)
        {
        case 3:
            eFormat = GL_RGB;
            break;
        case 4:
            eFormat = GL_RGBA;
            break;
        default:
            eFormat = GL_LUMINANCE;
            break;
        }

        gl->TexImage2D(context, GL_TEXTURE_2D, 0, eFormat, jpegWidth, jpegHeight, 0, eFormat, GL_UNSIGNED_BYTE, jpeg);

        gl->TexParameteri(context, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->TexParameteri(context, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


        Identity(jsonMeshTransform.camera);
        LookAt(jsonMeshTransform.camera,
            0.f,0.f,3.f,
            0.f,0.f,-5.f,
            0.f,1.f,0.f);

        LoadJSONMesh(psz_cube_json, &jsonMeshTransform, &jsonIdxCount, &jsonVtxCount, &jsonVBO, &jsonIBO);

        //gl->FrontFace(context, GL_CW);
        gl->Enable(context, GL_CULL_FACE);
}

void DemoRender(PP_Resource inContext, PPB_OpenGLES2* inGL)
{
    //char msg[128];
    uint64_t ui64ElapsedTime = GetElapsedTimeMS();
    uint64_t ui64DeltaTimeMS = ui64ElapsedTime - ui64BaseTimeMS;

    gl->ClearColor(context, 1.0f, 0.0f, 0.0f, 1.0f);
    gl->Clear(context, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->Enable(context, GL_DEPTH_TEST);

    ui64BaseTimeMS = ui64ElapsedTime;

    //sprintf(msg, "dt=%d", ui64DeltaTimeMS);

    //DBG_LOG(msg);

    //Six degrees per second
    angle += 0.006f * (ui64DeltaTimeMS);

    if(angle > 360)
    {
        angle = 0;
    }

    Identity(jsonMeshTransform.rot);

    Rotate(jsonMeshTransform.rot, 0.f,1.f,0.f, angle);

    gl->BindTexture(context, GL_TEXTURE_2D, uiDisplacementMapTexture);
    gl->UseProgram(context, GLProgram);

    DrawJSONMesh(&jsonMeshTransform, jsonVtxCount, jsonIdxCount, jsonVBO, jsonIBO);
}

void DemoUpdate()
{
}

void DemoHandleString(const char* str, const uint32_t ui32StrLength)
{
    uint32_t i = 0;
    char* nextStr = (char*)str;

    //Convert space delimited string of texels to integer values
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

