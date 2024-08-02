//
//  TMQC.h
//  TMQC
//
//  Created by  on 2022/5/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef TMQ_C_H
#define TMQ_C_H

#if defined(_WIN32) && defined(TMQ_DLL_EXPORTS)
#define TMQ_EXPORTS extern "C" _declspec(dllexport)
#else
#if __GNUC__ >= 4
#define TMQ_EXPORTS extern "C"  __attribute__((visibility("default")))
#else
#define TMQ_EXPORTS extern "C"
#endif  //  __GNUC__ >= 4
#endif  //  WIN32
// Default message priority, all priority values can be [0, 10]
#define PRIORITY_NORMAL     5

/*
 * This is tmq message callback function, which can receive the callback invoke.
 * @param data, the parameter data is a pointer to the binary data.
 * @param length, represent the length of binary data.
 * @param customId, user defined value, it can be a long type value, or a pointer.
 * @return void, this is a sample callback, can return nothing.
 */
typedef void (*TMQMessageCallback)(void *data, int length, TMQId customId);

/// tmq memory alloc and free
/**
 * This is an malloc function, allows developer to allocate storage space by tmq.
 * TMQ malloc and free function should be used as pair, they are use to avoid
 * potential risks from cross-library call.
 * @param len, the length used for malloc
 * @return a pointer to the storage address.
 */
TMQ_EXPORTS void *tmq_malloc(int length);

/*
 * This is the free function, it must be used as pair of the tmq_malloc.
 * @para data, a pointer to the data allocated by tmq_malloc.
 */
TMQ_EXPORTS void tmq_free(void *data);

/// tmq functions
/**
 * Subscribe a topic with a callback receiver and custom id.
 * @param topic, a string value of topic, the length of a topic should be
 *      less than the limit length by tmq topic.
 * @param receiver, a callback function pointer, refer TMQMessageCallback for detail
 * @param customId, a long value or a data pointer define by developer
 * @return a id for this subscribe, it can be used for next action, such as unsubscribe.
 */
TMQ_EXPORTS TMQId tmq_subscribe(const char *topic, TMQMessageCallback receiver, TMQId customId = 0);
/**
 * Cancel a subscribe. When the subscribe is on invoking, it will wait until finish. So before
 * invoke this function, you should stop the receiver first, which is more better.
 * @param subId, the id return by tmq_subscribe.
 * @return bool, a bool value indicate whether the invoke is success or not.
 */
TMQ_EXPORTS bool tmq_unsubscribe(TMQId subId);
/**
 * pick a message from tmq. This function will traversal the message queue, lookup the a message
 * with the topic. Every invoke will pick only one message. If you want pick all topic message, pick
 * again and again until it return invalid result(-1).
 * @param topic, a string pointer indicate the tmq topic.
 * @param data, a pointer of the data pointer, if success, the data pointer will be assigned a valid
 *  value.
 * @return int, a int value indicate the length of the data. -1 represent no message any more.
 */
TMQ_EXPORTS int tmq_pick(const char *topic, void **data);
/**
 * Publish a binary data to the topic, and return the message id.
 * @param topic, a topic the message will be add.
 *  The length of a topic is limited by TMQ_TOPIC_MAX_LENGTH
 * @param data, a pointer to the binary data.
 * @param length, the length of the binary data.
 * @param flag, the flag for the message. Different flag has special behaviour.
 *  TMQ_MSG_TYPE_PICK, a pick only message. These type of message will be obtained by pick only.
 *  TMQ_MSG_TYPE_DISPATCH, a dispatch only message. These type of message will be dispatched only.
 *  TMQ_MSG_TYPE_ALL, a normal message, will be consumed by pick or dispatch only once.
 *  Default value flag is TMQ_MSG_TYPE_ALL.
 * @return TMQId, a long type value represent the published message id.
 */
TMQ_EXPORTS TMQMsgId tmq_publish(const char *topic, void *data, int length, int flag = 0,
                                 int priority = PRIORITY_NORMAL);

/// For tmq context functions.
/// A context represent operations with multiple topics. This can be regarded as a view of the tmq
/// messages. A content can include several topics, publish or pick message to(from) tmq will be
/// success if the topic of the message is included in the context.
/**
 * Create a topic context with receiver; receiver can be null, which means no callback.
 * @param receiver, a TMQMessageCallback receiver used to receive context message.
 * @return TMQId, a long type value represent the context.
 */
TMQ_EXPORTS TMQId tmq_create_ctx(TMQMessageCallback receiver = nullptr);
/*
 * Add a topic to the context.
 * @param ctx, a long type parameter represent the context obtained by tmq_create_ctx
 * @param topic, a string pointer to a topic.
 * @return, long, return the original context.
 */
TMQ_EXPORTS TMQId tmq_ctx_subscribe(TMQId ctx, const char *topic);
/**
 * Cancel a topic from context, subscribe associated with this topic will be canceled too.
 * @param ctx, a long value represent a context.
 * @param topic, a topic to be unsubscribe.
 * @return TMQId, return the original context.
 */
TMQ_EXPORTS TMQId tmq_ctx_unsubscribe(TMQId ctx, const char *topic = nullptr);
/**
 * Enable persistent storage or not. If the parameter enable is true, the file must be specified to
 * a valid path, the file associated with the path must have read/write permissions. If the file is
 * not exist, it will be created. Besides, if the enable is false, and the persistent instance is
 * running, it will clear the persistence and clear its file at the same time.
 * @param enable, a boolean value indicate whether the persistent is enable or not.
 * @param file, a pointer to file path, max length limits to 255.
 * @param a boolean value for whether the persistent is enable or not.
 */
TMQ_EXPORTS bool tmq_enable_persistent(bool enable, const char *file);
/**
 * Pick message with this context
 * @param ctx, a long value represent a context.
 * @param data, a pointer to data pointer. if success, the data pointer will be assigned
 *  a valid pointer value
 * @return long, the length of the data.
 */
TMQ_EXPORTS TMQSize tmq_ctx_pick(TMQId ctx, void **data);
/**
 * Publish message to a ctx, can also use tmq_publish.
 * @param ctx, a long value represent a context.
 * @param data, a pointer to a binary data.
 * @param length, the length of the data.
 * @param flag, the flag of the message. Detail information refers to tmq_pick.
 * @return TMQMsgId, a TMQMsgId type value represent the id of the published message.
 */
TMQ_EXPORTS TMQMsgId tmq_ctx_publish(TMQId ctx, void *data, int length, int flag = 0,
                                     int priority = PRIORITY_NORMAL);
/**
 * Remove a context and destroy it.
 * @param ctx, a long value represent the context, created by tmq_create_ctx.
 * @return void, nothing to be returned.
 * Attentions: during destroying a context, the subscription in context will be release at the same
 * time. So, pay attention to the cancel of the subscription, refer to tmq_unsubscribe for detail.
 */
TMQ_EXPORTS void tmq_destroy_ctx(TMQId ctx);

#endif // TMQ_C_H
