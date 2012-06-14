#ifndef NACL_FILE_H
#define NACL_FILE_H

#include <string>
#include "naclcontext.h"

typedef void (*OnDownloaded)(std::string fileContents, void* pvUserData, int iSuccessful);

int StartDownload(NaCLContext* psNaCLContext,
            const char* url,
            void* pvUserData,
            OnDownloaded pfnOnDownloaded);

#endif
