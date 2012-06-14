#include "demo.h"
#include "maths.h"
#include "cJSON.h"

#include "textured_vs.h"
#include "textured_ps.h"

#include "jpgd.h"

#include <cstdlib>
#include <cctype>
#include <cstdio>
#include <sys/time.h>

#include "naclfile.h"

#define max(a, b) ((a > b) ? (a) : (b))

//Vertex input locations
const uint32_t VA_POSITION_INDEX = 0; //VA=Vertex array
const uint32_t VA_TEXCOORD_INDEX = 1;
const uint32_t VA_NORMAL_INDEX = 2;

const int iDispMapWidth = 256;
const int iDispMapHeight = 256;

typedef struct DownloadContext_TAG
{
    int id;
    PP_Resource hRenderContext;
    PPB_OpenGLES2* psGL;
    int iDownloaded;
} DownloadContext;

typedef struct Texture_TAG
{
    GLuint hTextureGL;
    GLubyte* image;
    int iWidth;
    int iHeight;
} Texture;

typedef struct
{
    float projection[4][4];
    float camera[4][4];
    float rot[4][4];
    float scale[4][4];
    float xform[4][4];
} Transformations;

typedef struct Mesh_TAG
{
    GLuint hVBO;
    GLuint hIBO;
    int iIdxCount;
    int iVtxCount;
} Mesh;

typedef struct DemoContext_TAG
{
    DownloadContext sDownloadBaseTexture;
    DownloadContext sDownloadMesh;

    Texture sDisplacementMap;
    Texture sBaseMap;

    GLuint hPassthroughShader;
    GLuint hDisplacementMapShader;

    Mesh sMesh;
    Transformations sMeshTransform;

    //Shader constants
    int iHighlightEnabled;
    float fDispCoeff;

    timeval startTime;
    uint64_t ui64BaseTimeMS;

    float fAngleY;
    float fAngleX;
    float fAngleModX;
    float fAngleModY;

} DemoContext;

DemoContext* psDemoContext = 0;

uint64_t GetElapsedTimeMS()
{
    timeval t;
    gettimeofday(&t, NULL);

    uint64_t secs  = t.tv_sec - psDemoContext->startTime.tv_sec;
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
        char aszMessage[300];
		char aszShaderLog[256];

		gl->GetShaderInfoLog(context, shaders[0], 256, 0, aszShaderLog);

        sprintf(aszMessage, DBG_LOG_PREFIX"Failed to vertex compile shader\n%s\n", aszShaderLog);

		DBG_LOG(aszMessage);
	}

	shaders[1] = gl->CreateShader(context, GL_FRAGMENT_SHADER);
	gl->ShaderSource(context, shaders[1], 1, &ps, 0);
	gl->CompileShader(context, shaders[1]);
	gl->GetShaderiv(context, shaders[1], GL_COMPILE_STATUS, &status);

	if(!status)
	{
        char aszMessage[300];
		char aszShaderLog[256];

		gl->GetShaderInfoLog(context, shaders[1], 256, 0, aszShaderLog);

        sprintf(aszMessage, DBG_LOG_PREFIX"Failed to pixel compile shader\n%s\n", aszShaderLog);

		DBG_LOG(aszMessage);
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
		DBG_LOG(DBG_LOG_PREFIX"Failed to link program\n");
	}

	return program;
}

void LoadJSONMesh(PP_Resource context,
                  PPB_OpenGLES2* gl,
                  const char* mesh,
                  Transformations* psMatrices,
                  int* piNumIndices,
                  int* piNumVertices,
                  GLuint* puiVBO,
                  GLuint* puiIBO)
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
        DBG_LOG(DBG_LOG_PREFIX"No root JSON");
        return;
    }

    psIndices = cJSON_GetObjectItem(psRoot,"idx");

    if(!psIndices)
    {
        DBG_LOG(DBG_LOG_PREFIX"No indices JSON");
        cJSON_Delete(psRoot);
        return;
    }

    psVertices = cJSON_GetObjectItem(psRoot,"vtxpos");
    if(!psVertices)
    {
        DBG_LOG(DBG_LOG_PREFIX"No vertices JSON");
        cJSON_Delete(psRoot);
        return;
    }

    psTexCoords = cJSON_GetObjectItem(psRoot,"texcoord");
    if(!psTexCoords)
    {
        DBG_LOG(DBG_LOG_PREFIX"No texcoords JSON");
        cJSON_Delete(psRoot);
        return;
    }
    
    psNormals = cJSON_GetObjectItem(psRoot,"normal");
    if(!psNormals)
    {
        DBG_LOG(DBG_LOG_PREFIX"No normals JSON");
        cJSON_Delete(psRoot);
        return;
    }

    psBoundingBox = cJSON_GetObjectItem(psRoot, "bndbox");
    if(!psBoundingBox)
    {
        DBG_LOG(DBG_LOG_PREFIX"No no bndbox JSON");
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
        pfTexCoords[vtx] = 1 - psTexC->valuedouble;

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

void DrawJSONMesh(PP_Resource context,
                  PPB_OpenGLES2* gl,
                  Transformations* psMatrices,
                  int iNumVertices,
                  int iNumIndices,
                  GLuint uiVBO,
                  GLuint uiIBO)
{
    float afTransform[4][4];//The final matrix
    float modelview[4][4];

    gl->UseProgram(context, psDemoContext->hDisplacementMapShader);

    Identity(afTransform);

    Identity(modelview);
    MultMatrix(modelview, psMatrices->rot, psMatrices->camera);
    MultMatrix(modelview, psMatrices->scale, modelview);
    MultMatrix(modelview, psMatrices->xform, modelview);
    MultMatrix(afTransform,  modelview, psMatrices->projection);


    gl->UniformMatrix4fv(context, gl->GetUniformLocation(context, psDemoContext->hDisplacementMapShader, "Transform"), 1, GL_FALSE, (float*)&afTransform[0][0]);
    gl->Uniform1i(context, gl->GetUniformLocation(context, psDemoContext->hDisplacementMapShader, "TextureBase"), 0);

    gl->Uniform1i(context, gl->GetUniformLocation(context, psDemoContext->hDisplacementMapShader, "DispMap"), 1);

    gl->Uniform1f(context, gl->GetUniformLocation(context, psDemoContext->hDisplacementMapShader, "DISPLACEMENT_COEFFICIENT"), psDemoContext->fDispCoeff);

    gl->ActiveTexture(context, GL_TEXTURE0+1);
    gl->BindTexture(context, GL_TEXTURE_2D, psDemoContext->sDisplacementMap.hTextureGL);
    gl->ActiveTexture(context, GL_TEXTURE0);
    gl->BindTexture(context, GL_TEXTURE_2D, psDemoContext->sBaseMap.hTextureGL);

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

std::string g_Mesh;

void MeshDownloaded(std::string fileContents, void* pvUserData, int iSuccessful)
{
    DownloadContext* psDownloadContext = (DownloadContext*)pvUserData;
    //PP_Resource context = psDownloadContext->hRenderContext;
    //PPB_OpenGLES2* gl = psDownloadContext->psGL;

    //char aszMessage[256];

    DBG_LOG(DBG_LOG_PREFIX"MeshDownload Start");

    if(!iSuccessful)
    {
        DBG_LOG(DBG_LOG_PREFIX"Failed to download mesh");
        return;
    }

    g_Mesh = fileContents;

    /*LoadJSONMesh(context,
        gl,
        fileContents.c_str(),
        &psDemoContext->sMeshTransform,
        &psDemoContext->sMesh.iIdxCount,
        &psDemoContext->sMesh.iVtxCount,
        &psDemoContext->sMesh.hVBO,
        &psDemoContext->sMesh.hIBO);*/

    psDownloadContext->iDownloaded = 1;

    //sprintf(aszMessage, DBG_LOG_PREFIX"Index count: %d Vertex count: %d\n", psDemoContext->sMesh.iIdxCount,
      //  psDemoContext->sMesh.iVtxCount);
    //DBG_LOG(aszMessage);

    DBG_LOG(DBG_LOG_PREFIX"MeshDownload End");
}

std::string g_JPEG;

void TextureDownloaded(std::string fileContents, void* pvUserData, int iSuccessful)
{
    DownloadContext* psDownloadContext = (DownloadContext*)pvUserData;
    //PP_Resource context = psDownloadContext->hRenderContext;
    //PPB_OpenGLES2* gl = psDownloadContext->psGL;

    DBG_LOG(DBG_LOG_PREFIX"TextureDonwloaded Start");

    if(!iSuccessful)
    {
        DBG_LOG(DBG_LOG_PREFIX"Failed to download texture");
        return;
    }

    g_JPEG = fileContents;

    psDownloadContext->iDownloaded = 1;

    DBG_LOG(DBG_LOG_PREFIX"TextureDownloaded End");
}

void DemoInit(NaCLContext* psNaCLContext, int width, int height)
{
    PP_Resource context = psNaCLContext->hRenderContext;
    PPB_OpenGLES2* gl = psNaCLContext->psGL;

    const float aspectRatio = (float)width / (float)height;
    const float fieldOfView = 45.0f;

    DBG_LOG(DBG_LOG_PREFIX"DemoInit Start");

    if(!psDemoContext)
    {
        psDemoContext = (DemoContext*)calloc(1, sizeof(DemoContext));

        //Start the async downloads early.

        DownloadContext* psTextureDownloadContext = &psDemoContext->sDownloadBaseTexture;//(DownloadContext*)malloc(sizeof(DownloadContext));
        psTextureDownloadContext->id = 0;
        psTextureDownloadContext->hRenderContext = context;
        psTextureDownloadContext->psGL = gl;
        psTextureDownloadContext->iDownloaded = 0;

        StartDownload(psNaCLContext, "dload/checkerboard.jpg", psTextureDownloadContext, TextureDownloaded);

        DownloadContext* psMeshDownloadContext = &psDemoContext->sDownloadMesh;//(DownloadContext*)malloc(sizeof(DownloadContext));
        psMeshDownloadContext->id = 1;
        psMeshDownloadContext->hRenderContext = context;
        psMeshDownloadContext->psGL = gl;
        psMeshDownloadContext->iDownloaded = 0;

        StartDownload(psNaCLContext, "dload/cube.json", psMeshDownloadContext, MeshDownloaded);
    }

    Identity(psDemoContext->sMeshTransform.projection);
    Persp(psDemoContext->sMeshTransform.projection, fieldOfView, aspectRatio,
        1.0, 1000.0);

    if(!psDemoContext->sDisplacementMap.image) //First time init has been called
    {
        int i;

        GLubyte* textureImage = (GLubyte*)malloc((iDispMapWidth*iDispMapHeight)*sizeof(GLubyte));

        for(i=0; i<(iDispMapWidth*iDispMapHeight); ++i)
        {
            textureImage[i] = 0;
        }

        DBG_LOG(DBG_LOG_PREFIX"DemoInit A");

        psDemoContext->sDisplacementMap.image = textureImage;

        gettimeofday(&psDemoContext->startTime, NULL);

        psDemoContext->ui64BaseTimeMS = GetElapsedTimeMS();
        psDemoContext->fAngleY = 0;
        psDemoContext->fAngleX = 0;
        //int iHighlightEnabled = 0;
        psDemoContext->fDispCoeff = 0.5f;

        DBG_LOG(DBG_LOG_PREFIX"DemoInit B");
    }

    psDemoContext->hDisplacementMapShader = CreateProgram(context, gl, psz_textured_vs, psz_textured_ps);

    //psDemoContext->hPassthroughShader = CreateProgram(context, gl, psz_passthrough_vs, psz_passthrough_ps);

    DBG_LOG(DBG_LOG_PREFIX"DemoInit C");

    gl->GenTextures(context, 1, &psDemoContext->sDisplacementMap.hTextureGL);

    gl->BindTexture(context, GL_TEXTURE_2D, psDemoContext->sDisplacementMap.hTextureGL);

    gl->TexImage2D(context, GL_TEXTURE_2D, 0, GL_LUMINANCE, iDispMapWidth, iDispMapHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, psDemoContext->sDisplacementMap.image);

    DBG_LOG(DBG_LOG_PREFIX"DemoInit D");

    gl->TexParameteri(context, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->TexParameteri(context, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    Identity(psDemoContext->sMeshTransform.camera);
    LookAt(psDemoContext->sMeshTransform.camera,
        0.f,0.f,3.f,
        0.f,0.f,-5.f,
        0.f,1.f,0.f);

    gl->Enable(context, GL_CULL_FACE);

    DBG_LOG(DBG_LOG_PREFIX"DemoInit End");
}

void DrawLoadingWheel()
{
    /*for(GLint i=30;i>0;--i)//build a circle with 30 line segments
    {
        cosine=static_cast<GLfloat>(cos(i*2*PI/30.0)*(PLAYER_WIDTH));
        sine=static_cast<GLfloat>(sin(i*2*PI/30.0)*(PLAYER_HEIGHT));
        
        pfVertices[k++] = cosine;
        pfVertices[k++] = sine;

        pfColour[m++] = 1;
        pfColour[m++] = 0;
        pfColour[m++] = 0;
    }*/
}

void DemoRender(NaCLContext* psNaCLContext)
{
    PP_Resource context = psNaCLContext->hRenderContext;
    PPB_OpenGLES2* gl = psNaCLContext->psGL;

    float rotY[4][4];
    float rotX[4][4];

    uint64_t ui64ElapsedTime = GetElapsedTimeMS();
    uint64_t ui64DeltaTimeMS = ui64ElapsedTime - psDemoContext->ui64BaseTimeMS;

    //DBG_LOG(DBG_LOG_PREFIX"DemoRender Start");

    gl->Enable(context, GL_DEPTH_TEST);

    psDemoContext->ui64BaseTimeMS = ui64ElapsedTime;

    if(!psDemoContext->sDownloadBaseTexture.iDownloaded ||
       !psDemoContext->sDownloadMesh.iDownloaded)
    {
        gl->ClearColor(context, 0.0, 0.0, 1.0, 1.0f);
        gl->Clear(context, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        DrawLoadingWheel();

        //DBG_LOG(DBG_LOG_PREFIX"DemoRender End A");
        return;
    }

    if(psDemoContext->sDownloadMesh.iDownloaded == 1)
    {
        LoadJSONMesh(context,
        gl,
        g_Mesh.c_str(),
        &psDemoContext->sMeshTransform,
        &psDemoContext->sMesh.iIdxCount,
        &psDemoContext->sMesh.iVtxCount,
        &psDemoContext->sMesh.hVBO,
        &psDemoContext->sMesh.hIBO);

        psDemoContext->sDownloadMesh.iDownloaded = 2;
    }

    if(psDemoContext->sDownloadBaseTexture.iDownloaded == 1)
    {
        unsigned char* jpeg = 0;
        int jpegWidth = 0;
        int jpegHeight = 0;
        int iNumComponents = 0;
        GLenum eFormat;

        gl->BindTexture(context, GL_TEXTURE_2D, psDemoContext->sBaseMap.hTextureGL);

        jpeg = jpgd::decompress_jpeg_image_from_memory((unsigned char*)g_JPEG.c_str(), g_JPEG.length(), 
                                       &jpegWidth, &jpegHeight, &iNumComponents, 3);

        if(!jpeg)
        {
            DBG_LOG(DBG_LOG_PREFIX"Bad jpeg texture");
            return;
        }

        switch(iNumComponents)
        {
            case 3:
            {
                eFormat = GL_RGB;
                break;
            }
            case 4:
            {
                eFormat = GL_RGBA;
                break;
            }
            default:
            {
                eFormat = GL_LUMINANCE;
                break;
            }
        }

        gl->TexImage2D(context, GL_TEXTURE_2D, 0, eFormat, jpegWidth, jpegHeight, 0, eFormat, GL_UNSIGNED_BYTE, jpeg);

        gl->TexParameteri(context, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->TexParameteri(context, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        psDemoContext->sDownloadBaseTexture.iDownloaded = 2;
    }
    gl->ClearColor(context, 0.5, 0.5, 0.5, 1.0f);
    gl->Clear(context, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Identity(psDemoContext->sMeshTransform.rot);

    Identity(rotX);
    Identity(rotY);

    psDemoContext->fAngleY += psDemoContext->fAngleModY * ui64DeltaTimeMS;
    psDemoContext->fAngleX += psDemoContext->fAngleModX * ui64DeltaTimeMS;

    Rotate(rotY, 0.f,1.f,0.f, psDemoContext->fAngleY);

    Rotate(rotX, 1.f,0.f,0.f, psDemoContext->fAngleX);

    MultMatrix(psDemoContext->sMeshTransform.rot, rotY, rotX);

    gl->BindTexture(context, GL_TEXTURE_2D, psDemoContext->sDisplacementMap.hTextureGL);
    gl->UseProgram(context, psDemoContext->hDisplacementMapShader);

    gl->Uniform1f(context, gl->GetUniformLocation(context, psDemoContext->hDisplacementMapShader, "HighlightEnabled"), psDemoContext->iHighlightEnabled);

    DrawJSONMesh(context,
        gl,
        &psDemoContext->sMeshTransform,
        psDemoContext->sMesh.iVtxCount,
        psDemoContext->sMesh.iIdxCount,
        psDemoContext->sMesh.hVBO,
        psDemoContext->sMesh.hIBO);

    //DBG_LOG(DBG_LOG_PREFIX"DemoRender End B");
}

void DemoHandleString(NaCLContext* psNaCLContext, const char* str, const uint32_t ui32StrLength)
{
    char* nextStr = (char*)str;

    if(!psDemoContext)
    {
        return;
    }

    if( (ui32StrLength > 5) &&
        (nextStr[0] == 'D') && 
        (nextStr[1] == 'C') &&
        (nextStr[2] == 'O') &&
        (nextStr[3] == 'E') &&
        (nextStr[4] == 'F'))
    {
        nextStr += 5;
        psDemoContext->fDispCoeff = atof(nextStr);
    }
    else
    if((ui32StrLength > 5) &&
        (nextStr[0] == 'T') && 
        (nextStr[1] == 'E') &&
        (nextStr[2] == 'X') &&
        (nextStr[3] == 'T') &&
        (nextStr[4] == 'U'))
    {
        nextStr += 5;

#if 1

        DBG_LOG(DBG_LOG_PREFIX"Updating dmap");

        uint32_t i = 0;

        //Convert space delimited string of texels to integer values
        nextStr = strtok(nextStr, " ");
        while((nextStr != 0) && (i < ui32StrLength))
        {
            psDemoContext->sDisplacementMap.image[i] = atoi(nextStr);
            nextStr = strtok(0, " ");

            ++i;
        }

        psNaCLContext->psGL->BindTexture(psNaCLContext->hRenderContext, GL_TEXTURE_2D, psDemoContext->sDisplacementMap.hTextureGL);
        psNaCLContext->psGL->TexSubImage2D(psNaCLContext->hRenderContext, GL_TEXTURE_2D, 0, 0, 0, iDispMapWidth, iDispMapHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, psDemoContext->sDisplacementMap.image);
#endif

#if 0
        while((i+5) < ui32StrLength)
        {
            psDemoContext->sDisplacementMap.image[i] = ((unsigned char*)nextStr)[i];
            ++i;
        }

        psNaCLContext->psGL->BindTexture(psNaCLContext->hRenderContext, GL_TEXTURE_2D, psDemoContext->sDisplacementMap.hTextureGL);
        psNaCLContext->psGL->TexSubImage2D(psNaCLContext->hRenderContext, GL_TEXTURE_2D, 0, 0, 0, iDispMapWidth, iDispMapHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, psDemoContext->sDisplacementMap.image);
#endif

#if 0
        int iWidth, iHeight, iNumComponents;
        GLenum eFormat;

        if(psDemoContext->sDisplacementMap.image)
        {
            free(psDemoContext->sDisplacementMap.image);
            psDemoContext->sDisplacementMap.image = 0;
        }

        psDemoContext->sDisplacementMap.image = jpgd::decompress_jpeg_image_from_memory((unsigned char*)nextStr, ui32StrLength-5, 
                                       &iWidth, &iHeight, &iNumComponents, 4);

        if(!psDemoContext->sDisplacementMap.image)
        {
            DBG_LOG(DBG_LOG_PREFIX"Bad jpeg texture");
            return;
        }

        switch(iNumComponents)
        {
            case 3:
            {
                eFormat = GL_RGB;
                break;
            }
            case 4:
            {
                eFormat = GL_RGBA;
                break;
            }
            default:
            {
                eFormat = GL_LUMINANCE;
                break;
            }
        }

        psNaCLContext->psGL->BindTexture(psNaCLContext->hRenderContext, GL_TEXTURE_2D, psDemoContext->sDisplacementMap.hTextureGL);
        psNaCLContext->psGL->TexSubImage2D(psNaCLContext->hRenderContext, GL_TEXTURE_2D, 0, 0, 0, iDispMapWidth, iDispMapHeight, eFormat, GL_UNSIGNED_BYTE, psDemoContext->sDisplacementMap.image);
#endif
    }
    else
    if((ui32StrLength == 5) &&
        (nextStr[0] == 'H') && 
        (nextStr[1] == 'I') &&
        (nextStr[2] == 'G') &&
        (nextStr[3] == 'H'))
    {
        psNaCLContext->psGL->UseProgram(psNaCLContext->hRenderContext, psDemoContext->hDisplacementMapShader);
        if(nextStr[4] == '1')
        {
            psDemoContext->iHighlightEnabled = 1;
        }
        else
        {
            psDemoContext->iHighlightEnabled = 0;
        }
    }
}

void DemoHandleKeyDown(uint32_t ui32KeyCode)
{
    switch(ui32KeyCode)
    {
        case 'W':
        {
            psDemoContext->fAngleModX = 0.03f;//30 degrees per second
            break;
        }
        case 'A':
        {
            psDemoContext->fAngleModY = -0.03f;
            break;
        }
        case 'S':
        {
            psDemoContext->fAngleModX = -0.03f;
            break;
        }
        case 'D':
        {
            psDemoContext->fAngleModY = 0.03f;
            break;
        }
    }
}

void DemoHandleKeyUp(uint32_t ui32KeyCode)
{
    switch(ui32KeyCode)
    {
        case 'W':
        {
            psDemoContext->fAngleModX = 0;
            break;
        }
        case 'A':
        {
            psDemoContext->fAngleModY = 0;
            break;
        }
        case 'S':
        {
            psDemoContext->fAngleModX = 0;
            break;
        }
        case 'D':
        {
            psDemoContext->fAngleModY = 0;
            break;
        }
    }
}

void DemoEnd()
{
    free(psDemoContext->sDisplacementMap.image);
    psDemoContext->sDisplacementMap.image = 0;

    free(psDemoContext);
    psDemoContext = NULL;
}

