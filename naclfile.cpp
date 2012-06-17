//Basically the same code from the geturl SDK example

#include "naclfile.h"
#include "naclcontext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int READ_BUFFER_SIZE = 1048576;//Stream in 1MB chunks.

class NaCLFile
{
public:
    NaCLContext* psNaCLContext;
    std::string filebody;
    char buffer[READ_BUFFER_SIZE];
    PP_Resource loader;
    PP_Resource request;
    OnDownloaded pfnOnDownloaded;
    void* pvUserData;
};

void OnRead(void* pvUserData, int32_t result);

void ReadBody(NaCLFile* psFile)
{
    PP_CompletionCallback fileReadCallback;
    fileReadCallback.func = OnRead;
    fileReadCallback.user_data = psFile;
    fileReadCallback.flags = PP_COMPLETIONCALLBACK_FLAG_OPTIONAL;

    int iResult = PP_OK;
    do
    {
        iResult = psFile->psNaCLContext->psURLLoader->ReadResponseBody(psFile->loader, psFile->buffer, READ_BUFFER_SIZE, fileReadCallback);
        
        // Handle streaming data directly. Note that we *don't* want to call
        // OnRead here, since in the case of result > 0 it will schedule
        // another call to this function. If the network is very fast, we could
        // end up with a deeply recursive stack.
        if (iResult > 0)
        {
            //AppendDataBytes(buffer, result);

            // Make sure we don't get a buffer overrun.
            iResult = std::min(READ_BUFFER_SIZE, iResult);
            // Note that we do *not* try to minimally increase the amount of allocated
            // memory here by calling url_response_body_.reserve().  Doing so causes a
            // lot of string reallocations that kills performance for large files.
            psFile->filebody.insert(psFile->filebody.end(),
                                    psFile->buffer,
                                    psFile->buffer + iResult);
        }
    } while (iResult > 0);

    if(iResult != PP_OK_COMPLETIONPENDING)
    {
        OnRead(psFile, iResult);
    }
}

void OnRead(void* pvUserData, int32_t result)
{
    NaCLFile* psFile = (NaCLFile*)pvUserData;

    if (result == PP_OK)
    {
        // Streaming the file is complete.
        psFile->pfnOnDownloaded(psFile->filebody, psFile->pvUserData, 1);
        delete psFile;

    }
    else if (result > 0)
    {
        // The URLLoader just filled "result" number of bytes into our buffer.
        // Save them and perform another read.

        // Make sure we don't get a buffer overrun.
        int iNumBytes = std::min(READ_BUFFER_SIZE, result);
        // Note that we do *not* try to minimally increase the amount of allocated
        // memory here by calling url_response_body_.reserve().  Doing so causes a
        // lot of string reallocations that kills performance for large files.
        psFile->filebody.insert(psFile->filebody.end(),
                                psFile->buffer,
                                psFile->buffer + iNumBytes);

        ReadBody(psFile);
    }
    else
    {
        // A read error occurred.
        psFile->pfnOnDownloaded(psFile->filebody, psFile->pvUserData, 0);
        delete psFile;
    }
}

void OnOpen(void* user_data, int32_t result)
{
    int64_t bytes_received = 0;
    int64_t total_bytes_to_be_received = 0;
    NaCLFile* psFile = (NaCLFile*)user_data;

    //Try to pre-allocate memory for the response.
    if (psFile->psNaCLContext->psURLLoader->GetDownloadProgress(psFile->loader,
                                        &bytes_received,
                                        &total_bytes_to_be_received))
    {
        if (total_bytes_to_be_received > 0)
        {
            psFile->filebody.reserve(total_bytes_to_be_received);
        }
    }

    // We will not use the download progress anymore, so just disable it.
    psFile->psNaCLContext->psURLRequest->SetProperty(psFile->request,
        PP_URLREQUESTPROPERTY_RECORDDOWNLOADPROGRESS, PP_MakeBool(PP_FALSE));
    
    ReadBody(psFile);
}

int StartDownload(NaCLContext* psNaCLContext,
            const char* url,
            void* pvUserData,
            OnDownloaded pfnOnDownloaded)
{
    PP_CompletionCallback fileOpenCallback;

    NaCLFile* psFile = new NaCLFile();

    psFile->psNaCLContext = psNaCLContext;

    psFile->loader = psNaCLContext->psURLLoader->Create(psNaCLContext->hModule);

    psFile->request = psNaCLContext->psURLRequest->Create(psNaCLContext->hModule);

    psFile->pfnOnDownloaded = pfnOnDownloaded;

    psFile->pvUserData = pvUserData;

    psNaCLContext->psURLRequest->SetProperty(psFile->request,
    PP_URLREQUESTPROPERTY_URL, psNaCLContext->psVar->VarFromUtf8(url, strlen(url)));

    psNaCLContext->psURLRequest->SetProperty(psFile->request,
        PP_URLREQUESTPROPERTY_METHOD, psNaCLContext->psVar->VarFromUtf8("GET", 3));

    psNaCLContext->psURLRequest->SetProperty(psFile->request,
        PP_URLREQUESTPROPERTY_RECORDDOWNLOADPROGRESS, PP_MakeBool(PP_TRUE));

    fileOpenCallback.func = OnOpen;
    fileOpenCallback.user_data = psFile;
    fileOpenCallback.flags = PP_COMPLETIONCALLBACK_FLAG_NONE;

    if(psNaCLContext->psURLLoader->Open(psFile->loader,
        psFile->request,
        fileOpenCallback) != PP_OK_COMPLETIONPENDING)
    {
        return 0;
    }

    return 1;
}
