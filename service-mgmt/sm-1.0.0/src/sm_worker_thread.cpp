//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_worker_thread.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "sm_util_types.h"
#include "sm_debug.h"
#include "sm_trap.h"

SmSimpleAction::SmSimpleAction(const char* action_name, SmSimpleActionCallback callback)
{
    _callback = callback;
    strncpy(_action_name, action_name, sizeof(_action_name));
}

SmSimpleAction::~SmSimpleAction()
{
}

void SmSimpleAction::action()
{
    if(NULL != _callback)
    {
        _callback(*this);
    }
}

const char* SmSimpleAction::get_name() const
{
    return _action_name;
}

SmWorkerThread SmWorkerThread::_the_worker;

// ****************************************************************************
// SmWorkerThread::get_worker get the thread singleton object
// ****************************************************************************
SmWorkerThread& SmWorkerThread::get_worker()
{
    return SmWorkerThread::_the_worker;
}
// ****************************************************************************

// ****************************************************************************
// SmWorkerThread::initialize initialize the worker thread
// ****************************************************************************
SmErrorT SmWorkerThread::initialize()
{
    SmWorkerThread::get_worker().go();
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// SmWorkerThread::finalize finalize the worker thread
// ****************************************************************************
SmErrorT SmWorkerThread::finalize()
{
    SmWorkerThread::get_worker().stop();
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// SmWorkerThread::SmWorkerThread
// ****************************************************************************
SmWorkerThread::SmWorkerThread() : _priority_queue(), _regular_queue()
{
    this->_mutex = PTHREAD_MUTEX_INITIALIZER;
    this->_goon = true;
    this->_thread_created = false;
}
// ****************************************************************************

// ****************************************************************************
// SmWorkerThread::~SmWorkerThread
// ****************************************************************************
SmWorkerThread::~SmWorkerThread()
{
    sem_destroy(&this->_sem);
}
// ****************************************************************************

// ****************************************************************************
// SmWorkerThread::thread_helper helper function for the thread entry
// ****************************************************************************
void* SmWorkerThread::thread_helper(SmWorkerThread* workerThread)
{
    pthread_setname_np( pthread_self(), "worker" );
    sm_debug_set_thread_info();
    sm_trap_set_thread_info();
    DPRINTFI("worker thread started");
    workerThread->thread_run();
    return 0;
}
// ****************************************************************************

// ****************************************************************************
// SmWorkerThread::go prepare and start the thread
// ****************************************************************************
SmErrorT SmWorkerThread::go()
{
    if(this->_thread_created)
    {
        DPRINTFE("Worker thread has already been created");
        return SM_FAILED;
    }

    this->_thread_created = true;
    if( 0 != sem_init(&this->_sem, 0, 0) )
    {
        DPRINTFE("Cannot init semaphore");
        return SM_FAILED;
    }

    int result = pthread_create( &this->_thread, NULL,
                             (void*(*)(void*))SmWorkerThread::thread_helper, (void*)this );
    if( 0 != result )
    {
        DPRINTFE("Failed to create thread. Error %d", errno);
        return SM_FAILED;
    }
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// SmWorkerThread::stop stop the worker thread
// ****************************************************************************
SmErrorT SmWorkerThread::stop()
{
    this->_goon = false;
    void* result = NULL;
    int res = pthread_join(this->_thread, &result);

    if(0 != res)
    {
        DPRINTFE("pthread_join failed. Error %d", res);
    }
    if(NULL != result)
    {
        free(result);
    }
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// SmWorkerThread::add_action add a regular priority action
// ****************************************************************************
void SmWorkerThread::add_action(SmAction* action)
{
    mutex_holder(&this->_mutex);
    this->_regular_queue.push(action);
    int res = sem_post(&_sem);
    if(0 != res)
    {
        DPRINTFE("failed to signal semaphore. error %d", errno);
    }

    DPRINTFI("Action %s is added", action->get_name());
}
// ****************************************************************************

// ****************************************************************************
// SmWorkerThread::add_priority_action add a high priority action
// ****************************************************************************
void SmWorkerThread::add_priority_action(SmAction* action)
{
    mutex_holder(&this->_mutex);
    this->_priority_queue.push(action);
    int res = sem_post(&_sem);
    if(0 != res)
    {
        DPRINTFE("failed to signal semaphore. error %d", errno);
    }
    DPRINTFI("Action %s is added to priority queue", action->get_name());
}
// ****************************************************************************

// ****************************************************************************
// SmWorkerThread::thread_run main loop of the worker thread
// ****************************************************************************
void SmWorkerThread::thread_run()
{
    int wait_interval_ns = 50*1000000; //50 ms
    int ns_to_sec = 1000000000;
    struct timespec wait_time;

    while(this->_goon)
    {
        clock_gettime(CLOCK_REALTIME, &wait_time);

        wait_time.tv_nsec += wait_interval_ns;
        if(wait_time.tv_nsec > ns_to_sec -1)
        {
            wait_time.tv_nsec -= ns_to_sec;
            wait_time.tv_sec ++;
        }

        if(0 ==  sem_timedwait(&this->_sem, &wait_time))
        {
            SmAction* action = NULL;
            if(!this->_priority_queue.empty())
            {
                action = this->_priority_queue.front();
                this->_priority_queue.pop();
            }else if(!this->_regular_queue.empty())
            {
                action = this->_regular_queue.front();
                this->_regular_queue.pop();
            }
            if( NULL != action )
            {
                DPRINTFI("Executing action %s", action->get_name());
                action->action();
                DPRINTFI("Executing action %s completed", action->get_name());
            }
        }else if(ETIMEDOUT != errno)
        {
            DPRINTFE("Semaphore wait failed. Error %d", errno);
        }
    }
}
// ****************************************************************************
