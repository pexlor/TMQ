//
//  TMQFactory.h
//  TMQFactory
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_FACTORY_H
#define TMQ_FACTORY_H

#if defined(_WIN32) && defined(TMQ_DLL_EXPORTS)
#define TMQ_EXPORTS _declspec(dllexport)
#else
#define TMQ_EXPORTS
#endif // WIN32

#include "TMQTopic.h"

/**
 * TMQ topic instance factory, use to create TMQTopic.
 */
class TMQFactory {
public:
    /**
     * Thread safe method for creating tmq topic instance which is singleton.
     * @return TMQTopic*, a pointer to the TMQTopic instance.
     */
    TMQ_EXPORTS static TMQTopic *GetTopicInstance();
};

#endif //TMQ_FACTORY_H
