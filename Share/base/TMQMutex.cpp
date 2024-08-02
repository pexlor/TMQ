#include "TMQMutex.h"

TMQMutex::TMQMutex()
{
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
#if _WINDOWS
#else
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
#endif
    int r = pthread_mutex_init(&mutex, &mutexAttr);
    pthread_mutexattr_destroy(&mutexAttr);
}
bool TMQMutex::Lock()
{
    return pthread_mutex_lock(&mutex) == 0;
}
bool TMQMutex::UnLock()
{
    return pthread_mutex_unlock(&mutex) == 0;
}