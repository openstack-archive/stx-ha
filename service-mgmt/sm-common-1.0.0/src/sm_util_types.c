//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <pthread.h>
#include <stdlib.h>
#include "sm_util_types.h"
#include "sm_debug.h"

mutex_holder::mutex_holder(pthread_mutex_t* mutex)
{
    this->_mutex = mutex;
    if( 0 != pthread_mutex_lock( this->_mutex ) )
    {
        DPRINTFE("Critical error, failed to obtain the mutex. Quit...");
        abort();
    }
}

mutex_holder::~mutex_holder()
{
    if( 0 != pthread_mutex_unlock( this->_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
    }
}
