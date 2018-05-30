//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_uuid.h"

#include <string.h>
#include <uuid/uuid.h>

#include "sm_types.h"
#include "sm_debug.h"

// ****************************************************************************
// Universally Unique Identifier - Create
// ======================================
void sm_uuid_create( SmUuidT uuid )
{
    uuid_t uu;

    memset( uuid, 0, sizeof(SmUuidT) );
    uuid_generate( uu );
    uuid_unparse_lower( uu,  uuid );
}
// ****************************************************************************

// ****************************************************************************
// Universally Unique Identifier - Initialize
// ==========================================
SmErrorT sm_uuid_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Universally Unique Identifier - Finalize
// ========================================
SmErrorT sm_uuid_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
