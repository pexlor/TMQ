#ifndef ANDROID_TMQFACTORY_H
#define ANDROID_TMQFACTORY_H

#if defined(_WIN32) && defined(TMQ_DLL_EXPORTS)
#define TMQ_EXPORTS _declspec(dllexport)
#else
#define TMQ_EXPORTS
#endif // WIN32

#include "TMQTopic.h"

class TMQFactory
{
public:
    TMQ_EXPORTS static TMQTopic *GetTopicInstance();
};

#endif // ANDROID_TMQFACTORY_H