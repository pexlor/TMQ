#include "pthread.h"

// 定义PExecutor类
class PExecutor {
public:
    // 定义pthread_t类型的tid成员变量，用于存储线程ID
    pthread_t tid;
    // 定义pthread_mutex_t类型的mutex成员变量，用于线程间互斥访问共享资源
    pthread_mutex_t mutex;
    // 定义pthread_cond_t类型的cond成员变量，用于线程间的条件变量同步
    pthread_cond_t cond;

public:
    // PExecutor类的构造函数
    PExecutor();
    // PExecutor类的析构函数
    ~PExecutor();
};

// 定义Executor为指向PExecutor的指针
#define Executor PExecutor*

#include "ThreadExecutor.h"
// 使用TMQ_NAMESPACE命名空间
USING_TMQ_NAMESPACE

// ThreadExecutor类的构造函数，初始化时根据传入的可调用对象创建线程
ThreadExecutor::ThreadExecutor(const TMQCallable *callable) : thread(nullptr) {
    // 如果可调用对象为空，则设置状态为结束并返回
    if (callable == nullptr) {
        state = EXECUTOR_STATE_END;
        return;
    }
    // 将传入的可调用对象指针保存为类成员变量
    this->callable = const_cast<TMQCallable *>(callable);
    // 设置状态为初始化
    state = EXECUTOR_STATE_INIT;
    // 创建PExecutor实例来管理线程
    thread = new PExecutor();
}

// ThreadExecutor类的析构函数，确保资源被正确释放
ThreadExecutor::~ThreadExecutor() {
    // 设置状态为结束
    SetState(EXECUTOR_STATE_END);
}

// 获取ThreadExecutor的状态，确保在互斥锁保护下进行读取
TMQExecutorState ThreadExecutor::GetState() {
    TMQExecutorState localState;
    pthread_mutex_lock(&thread->mutex);
    localState = state;
    pthread_mutex_unlock(&thread->mutex);
    return localState;
}

// 设置ThreadExecutor的状态，确保在互斥锁保护下进行写入
void ThreadExecutor::SetState(TMQExecutorState executorState) {
    pthread_mutex_lock(&thread->mutex);
    this->state = executorState;
    pthread_mutex_unlock(&thread->mutex);
}

// 线程执行的入口函数
void* OnExecute(void *executor) {
    // 将void*类型的参数转换为ThreadExecutor*类型
    auto* threadExecutor = (ThreadExecutor*)executor;
    // 设置状态为就绪
    threadExecutor->SetState(EXECUTOR_STATE_READY);
    // 循环直到状态变为结束
    while (threadExecutor->GetState() != EXECUTOR_STATE_END) {
        // 设置状态为运行
        threadExecutor->SetState(EXECUTOR_STATE_RUNNING);
        // 调用可调用对象的OnExecute方法
#if _WINDOWS
        threadExecutor->GetCallable()->OnExecute((long)threadExecutor->thread->tid.x);
#else
        threadExecutor->GetCallable()->OnExecute((long)threadExecutor->thread->tid);
#endif
        // 在互斥锁保护下检查状态，如果是运行状态则等待条件变量
        pthread_mutex_lock(&threadExecutor->thread->mutex);
        if (threadExecutor->GetState() == EXECUTOR_STATE_RUNNING) {
            threadExecutor->SetState(EXECUTOR_STATE_WAITING);
            pthread_cond_wait(&threadExecutor->thread->cond, &threadExecutor->thread->mutex);
        }
        pthread_mutex_unlock(&threadExecutor->thread->mutex);
    }
    // 线程结束前删除PExecutor实例
    delete threadExecutor->thread;
    // 返回空指针
    return nullptr;
}

// 唤醒ThreadExecutor线程的方法
bool ThreadExecutor::Wakeup() {
    // 如果状态为结束，则不进行任何操作
    if (GetState() == EXECUTOR_STATE_END) {
        return false;
    }
    bool success = false;
    pthread_mutex_lock(&thread->mutex);
    // 检查线程是否未创建
#if _WINDOWS
    bool nullThread = (thread->tid.p == nullptr);
#else
    bool nullThread = (thread->tid == 0);
#endif
    // 如果线程未创建，则创建线程
    if (nullThread) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&thread->tid, &attr, OnExecute, this);
        pthread_attr_destroy(&attr);
        // 设置状态为启动中
        state = EXECUTOR_STATE_STARTING;
        success = true;
    } else if (state == EXECUTOR_STATE_WAITING) {
        // 如果线程处于等待状态，则发送信号唤醒线程
        pthread_cond_signal(&thread->cond);
        success = true;
    }
    pthread_mutex_unlock(&thread->mutex);
    return success;
}

// 获取ThreadExecutor的可调用对象
TMQCallable *ThreadExecutor::GetCallable() {
    return callable;
}

// 创建ThreadExecutor实例的静态方法
TMQExecutor *ThreadExecutor::CreateExecutor(TMQCallable *callable) {
    // 创建ThreadExecutor实例并返回
    auto* executor = new ThreadExecutor(callable);
    return executor;
}

// 释放ThreadExecutor实例的静态方法
void ThreadExecutor::ReleaseExecutor(TMQExecutor *executor) {
    // 如果传入的执行器不为空，则调用其Stop方法后删除
    if (executor) {
        auto* threadExecutor = (ThreadExecutor*)executor;
        threadExecutor->Stop();
    }
}

// 停止ThreadExecutor线程的方法
void ThreadExecutor::Stop() {
    // 设置状态为结束
    pthread_mutex_lock(&thread->mutex);
    state = EXECUTOR_STATE_END;
    // 发送信号唤醒可能正在等待的线程
    pthread_cond_signal(&thread->cond);
    pthread_mutex_unlock(&thread->mutex);
}

// PExecutor类的构造函数，初始化互斥锁和条件变量
PExecutor::PExecutor() : mutex{0}, cond{0} {
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    // 设置互斥锁类型为递归锁
    pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_RECURSIVE);
    // 初始化互斥锁
    pthread_mutex_init(&mutex, &mutexAttr);
    // 销毁互斥锁属性对象
    pthread_mutexattr_destroy(&mutexAttr);
    // 初始化条件变量
    pthread_cond_init(&cond, nullptr);
#ifdef _WIN32
    // 在Windows平台上初始化tid为默认值
    tid = {};
#else // _WIN32
    // 在非Windows平台上初始化tid为0
    tid = 0;
#endif
}

// PExecutor类的析构函数，销毁互斥锁和条件变量
PExecutor::~PExecutor() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}