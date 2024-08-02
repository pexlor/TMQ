#ifndef ANDROID_THREADEXECUTOR_H
#define ANDROID_THREADEXECUTOR_H

#include "TMQExecutor.h"
#include "Defines.h"
// 使用TMQ命名空间
TMQ_NAMESPACE

// 如果没有定义Executor，则在此处定义Executor为void*类型
#ifndef Executor
#define Executor void*
#endif

// 定义默认的最大阻塞计数
#define MAX_BLOCKED_COUNT_DEFAULT 1024

// 定义ThreadExecutor类，继承自TMQExecutor
class ThreadExecutor : public TMQExecutor {
private:
    // 私有构造函数和析构函数，防止外部直接创建和删除实例
    ThreadExecutor(const TMQCallable* callable);
    ~ThreadExecutor();
    // 保存传入的可调用对象指针
    TMQCallable* callable;
    // 保存线程执行器的状态
    volatile TMQExecutorState state;

public:
    // 线程成员变量，类型为Executor（即void*）
    Executor thread;
    // 获取可调用对象的方法
    TMQCallable* GetCallable();
    // 设置线程执行器的状态
    void SetState(TMQExecutorState state);
    // 停止线程执行器的方法
    void Stop();

public:
    // 获取线程执行器的状态
    virtual TMQExecutorState GetState();
    // 唤醒线程执行器的方法
    virtual bool Wakeup();

public:
    // 静态方法，用于创建ThreadExecutor实例
    static TMQExecutor* CreateExecutor(TMQCallable* callable);
    // 静态方法，用于释放ThreadExecutor实例
    static void ReleaseExecutor(TMQExecutor* executor);
};

// 结束TMQ命名空间的使用
TMQ_NAMESPACE_END

#endif // ANDROID_THREADEXECUTOR_H