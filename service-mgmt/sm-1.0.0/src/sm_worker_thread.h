//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_WORKER_THREAD_H__
#define __SM_WORKER_THREAD_H__

#include "sm_types.h"
#include <queue>
#include <pthread.h>
#include <semaphore.h>

// ****************************************************************************
// SmAction interface, action to process in a worker thread
// ****************************************************************************
class SmAction
{
    public:
        virtual void action() = 0;
        virtual const char* get_name() const = 0;
};

class SmSimpleAction : public SmAction
{
    public:
        typedef void(*SmSimpleActionCallback)(SmSimpleAction&);
        SmSimpleAction(const char* action_name, SmSimpleActionCallback callback);
        virtual ~SmSimpleAction();
        void action();
        const char* get_name() const;
    private:
        SmSimpleActionCallback _callback;
        char _action_name[32];
};

typedef std::queue<SmAction*> SmActionQueueT;

// ****************************************************************************
// SmWorkerThread work thread class
// ****************************************************************************
class SmWorkerThread
{
    public:
        SmWorkerThread();
        virtual ~SmWorkerThread();

        /* Add an action to the normal priority FCFS queue.
           A normal priority action will be scheduled after all
           high priority actions.
        */
        void add_action(SmAction* action);
        /*
           Add an action to high priority FCFS queue.
           A high priority action is nonpreemptive. It will
           be scheduled after the current action.
        */
        void add_priority_action(SmAction* action);


        // retrieve singleton object
        static SmWorkerThread& get_worker();
        // initialize worker thread
        static SmErrorT initialize();
        // stop worker thread
        static SmErrorT finalize();

    private:
        pthread_mutex_t _mutex;
        SmActionQueueT _priority_queue;
        SmActionQueueT _regular_queue;
        sem_t _sem;
        pthread_t _thread;
        bool _thread_created;
        bool _goon;

        // create worker thread and run tasks
        SmErrorT go();
        // stop processing tasks and stop the worker thread
        SmErrorT stop();

        // run the thread
        void thread_run();
        /* help function to provide function pointer for
           creating the worker thread
        */
        static void* thread_helper(SmWorkerThread* me);

        static SmWorkerThread _the_worker;

};

#endif //__SM_WORKER_THREAD_H__