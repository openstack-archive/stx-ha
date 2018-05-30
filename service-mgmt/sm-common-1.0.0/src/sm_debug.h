//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DEBUG_H__
#define __SM_DEBUG_H__

#include <stdbool.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SM_DEBUG_LOG,
    SM_DEBUG_SCHED_LOG,
} SmDebugLogTypeT;

typedef enum
{
    SM_DEBUG_LOG_LEVEL_ERROR,
    SM_DEBUG_LOG_LEVEL_INFO,
    SM_DEBUG_LOG_LEVEL_DEBUG,
    SM_DEBUG_LOG_LEVEL_VERBOSE,
} SmDebugLogLevelT;

// ****************************************************************************
// Debug - Log Level String
// ========================
extern const char* sm_debug_log_level_str( SmDebugLogLevelT level );
// ****************************************************************************

// ****************************************************************************
// Debug - Set Log Level
// =====================
extern void sm_debug_set_log_level( SmDebugLogLevelT level );
// ****************************************************************************

// ****************************************************************************
// Debug - Do Log
// ==============
extern bool sm_debug_do_log( const char* file_name, SmDebugLogLevelT level );
// ****************************************************************************

// ****************************************************************************
// Debug - Log
// ===========
extern void sm_debug_log( SmDebugLogTypeT type, const char* format, ... )
__attribute__ ((format (printf, 2, 3)));
// format string is parameter #2 and follows the printf formatting,
// variable arguments start at parameter #3
// ****************************************************************************

// ****************************************************************************
// Debug - Get Thread Information
// ==============================
extern const char* sm_debug_get_thread_info( void );
// ****************************************************************************

// ****************************************************************************
// Debug - Set Thread Information
// ==============================
extern void sm_debug_set_thread_info( void );
// ****************************************************************************

// ****************************************************************************
// Debug - Scheduler Log
// =====================
extern void sm_debug_sched_log( SmDebugLogTypeT type, const char* format, ... );
// ****************************************************************************

// ****************************************************************************
// Debug - Scheduler Log Start
// ===========================
extern void sm_debug_sched_log_start( char* domain );
// ****************************************************************************

// ****************************************************************************
// Debug - Scheduler Log Done
// ==========================
extern void sm_debug_sched_log_done( char* domain );
// ****************************************************************************

// ****************************************************************************
// Debug - Initialize
// ==================
extern SmErrorT sm_debug_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Debug - Finalize
// ================
extern SmErrorT sm_debug_finalize( void );
// ****************************************************************************

#define DPRINTF( level, format, args... ) \
    if( sm_debug_do_log( __FILE__, level ) ) \
        sm_debug_log( SM_DEBUG_LOG, "%s: %s: %s(%i): " format, \
                      sm_debug_log_level_str( level ), \
                      sm_debug_get_thread_info(), \
                      __FILE__, __LINE__, ##args )

#define DPRINTFE( format, args... ) \
    DPRINTF( SM_DEBUG_LOG_LEVEL_ERROR, format, ##args )
#define DPRINTFI( format, args... ) \
    DPRINTF( SM_DEBUG_LOG_LEVEL_INFO, format, ##args )
#define DPRINTFD( format, args... ) \
    DPRINTF( SM_DEBUG_LOG_LEVEL_DEBUG, format, ##args )
#define DPRINTFV( format, args... ) \
    DPRINTF( SM_DEBUG_LOG_LEVEL_VERBOSE, format, ##args )

#define SCHED_LOG_START( domain, format, args... ) \
    sm_debug_sched_log_start( domain ); \
    sm_debug_sched_log( SM_DEBUG_SCHED_LOG, "%s: " format, domain, ##args )

#define SCHED_LOG( domain, format, args... ) \
    sm_debug_sched_log( SM_DEBUG_SCHED_LOG, "%s: " format, domain, ##args )

#define SCHED_LOG_DONE( domain, format, args... ) \
    sm_debug_sched_log( SM_DEBUG_SCHED_LOG, "%s: " format, domain, ##args ); \
    sm_debug_sched_log_done( domain )

#ifdef __cplusplus
}
#endif

#endif // __SM_DEBUG_H__
