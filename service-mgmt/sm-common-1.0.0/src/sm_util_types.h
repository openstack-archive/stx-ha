//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_UTIL_TYPES_H__
#define __SM_UTIL_TYPES_H__

#include <pthread.h>

class mutex_holder
{
    private:
        pthread_mutex_t* _mutex;
    public:
        mutex_holder(pthread_mutex_t* mutex);
        virtual ~mutex_holder();
};

#endif //__SM_UTIL_TYPES_H__