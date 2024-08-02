#ifndef _TMQC_H
#define _TMQC_H

#if defined(_WIN32) && defined(TMQ_DLL_EXPORTS)
#define TMQ_EXPORTS extern "C" _declspec(dllexport)
#else
#if __GNUC__ >= 4
#define TMQ_EXPORTS extern "C" __attribute__((visibility("default")))
#else
#define TMQ_EXPORTS extern "C"
#endif //__GNUC__ >= 4
#endif // WIN32

#define TMQ_PRIORITY 4

typedef void (*TMQMessageCallback)(void *data, long length, long customId);

// tmq memory alloc and free
TMQ_EXPORTS void *tmq_malloc(unsigned long len);
TMQ_EXPORTS void tmq_free(void *data);
// tmq functions
TMQ_EXPORTS long tmq_subscribe(const char *topic, TMQMessageCallback receiver, long customId = 0);
TMQ_EXPORTS bool tmq_unsubscribe(long subId);
TMQ_EXPORTS long tmq_pick(const char *topic, void **data);
TMQ_EXPORTS long tmq_publish(const char *topic, void *data, long length, int flag = 0, int priority = TMQ_PRIORITY);
// for context
// create a topic context with receiver; receiver can be null, no callback
TMQ_EXPORTS long tmq_create_ctx(TMQMessageCallback receiver = nullptr);
// add a topic to context
TMQ_EXPORTS long tmq_ctx_subscribe(long ctx, const char *topic);
// cancel a topic from context, subscribe associated with this topic will be canceled too.
TMQ_EXPORTS long tmq_ctx_unsubscribe(long ctx, const char *topic = nullptr);
// pick message with this context
TMQ_EXPORTS long tmq_ctx_pick(long ctx, void **data);
// publish message to ctx, can also use tmq_publish
TMQ_EXPORTS long tmq_ctx_publish(long ctx, void *data, long length, int flag = 0, int priority = TMQ_PRIORITY);
// remove a context
TMQ_EXPORTS void tmq_destroy_ctx(long ctx);

#endif // _TMQC_H