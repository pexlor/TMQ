//
//  Executor.h
//  Executor
//
//  Created by  on 2022/3/28.
//  Copyright (c)  Tencent. All rights reserved.
//

#ifndef EXECUTOR_H
#define EXECUTOR_H

/**
 * An interface definition for a executor. This interface usually implemented by platform thread.
 */

/**
 * The state of an executor.
 */
enum TMQExecutorState {
    // Initialized state. In this state, the executor is creating.
    EXECUTOR_STATE_INIT,
    // Starting state. In this state, the thread in an executor is creating.
    EXECUTOR_STATE_STARTING,
    // Ready state, In this state, the executor is ready to running.
    EXECUTOR_STATE_READY,
    // Running state, In this state, the executor is on running, the OnExecute function is invoking.
    EXECUTOR_STATE_RUNNING,
    // Waiting state, In this state the executor is sleeping.
    EXECUTOR_STATE_WAITING,
    // Ending state, In this state, the executor is waiting for ending.
    EXECUTOR_STATE_ENDING,
    // Release state, In this state, the executor is release any resources, No code to run any more.
    EXECUTOR_STATE_RELEASE
};

/**
 * Interface definition for callback of the platform thread.
 */
class TMQCallable {
public:
    /**
     * Called by the platform thread.
     * @param eid, an long value to identify the thread.
     * @return bool, a boolean value indicate whether to call this method again.
     */
    virtual bool OnExecute(long eid) = 0;
};

/**
 * Interface definition for an executor. In this interface, there are two important APIs: GetState
 * and Wakeup. The method GetState can get the current state of the state. The Wakeup method can try
 * to wake up this executor to call the TMQCallable.
 */
class IExecutor {
public:
    /**
     * Get the state of the executor.
     * @return enum value, refer TMQExecutorState for detail.
     */
    virtual TMQExecutorState GetState() = 0;

    /**
     * Wake up the executor to run TMQCallable.
     * @return a boolean value indicate whether this call is success or not.
     */
    virtual bool Wakeup() = 0;

    /**
     * Default destructor for this virtual class.
     */
    virtual ~IExecutor() {}
};


#endif //EXECUTOR_H
