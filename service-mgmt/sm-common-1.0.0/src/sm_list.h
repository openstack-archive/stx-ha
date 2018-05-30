//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_LIST_H__
#define __SM_LIST_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef GList SmListT;

typedef GCompareFunc SmListCompareFunctionT;

#define SM_POINTER_TO_INT( pointer ) GPOINTER_TO_INT( pointer )
#define SM_INT_TO_POINTER( integer ) GINT_TO_POINTER( integer )

#define SM_POINTER_TO_UINT( pointer ) GPOINTER_TO_UINT( pointer )
#define SM_UINT_TO_POINTER( integer ) GUINT_TO_POINTER( integer )

typedef gpointer SmListEntryDataPtrT;

// ****************************************************************************
// List - Length
// =============
#define SM_LIST_LENGTH( list ) \
    g_list_length( list )
// ****************************************************************************

// ****************************************************************************
// List - For-Each
// ===============
#define SM_LIST_FOREACH( list, entry, var )            \
    for( entry = g_list_first(list),                   \
         var = ((NULL == entry) ? NULL : entry->data); \
         NULL != entry;                                \
         entry = g_list_next(entry),                   \
         var = ((NULL == entry) ? NULL : entry->data) )
// ****************************************************************************

// ****************************************************************************
// List - For-Each Safe
// ====================
#define SM_LIST_FOREACH_SAFE( list, entry, next, var )          \
    for( entry = g_list_first(list),                            \
         next  = ((NULL == entry) ? NULL : g_list_next(entry)), \
         var   = ((NULL == entry) ? NULL : entry->data);        \
         NULL != entry;                                         \
         entry = next,                                          \
         next  = ((NULL == entry) ? NULL : g_list_next(entry)), \
         var   = ((NULL == entry) ? NULL : entry->data) )
// ****************************************************************************

// ****************************************************************************
// List - Sort
// ===========
#define SM_LIST_SORT( list, compare_function ) \
    list = g_list_sort( list, compare_function )
// ****************************************************************************

// ****************************************************************************
// List - First
// ============
#define SM_LIST_FIRST( list, entry, var ) \
    entry = g_list_first( list ),         \
    var = ((NULL == entry) ? NULL : entry->data);
// ****************************************************************************

// ****************************************************************************
// List - Last
// ===========
#define SM_LIST_LAST( list, entry, var ) \
    entry = g_list_last( list ),         \
    var = ((NULL == entry) ? NULL : entry->data);
// ****************************************************************************

// ****************************************************************************
// List - Prepend
// ==============
#define SM_LIST_PREPEND( list, entry )       \
    if( NULL == g_list_find( list, entry ) ) \
        list = g_list_prepend( list, entry );
// ****************************************************************************

// ****************************************************************************
// List - Append
// =============
#define SM_LIST_APPEND( list, entry )        \
    if( NULL == g_list_find( list, entry ) ) \
        list = g_list_append( list, entry );
// ****************************************************************************

// ****************************************************************************
// List - Remove
// =============
#define SM_LIST_REMOVE( list, entry ) \
    list = g_list_remove( list, entry );
// ****************************************************************************

// ****************************************************************************
// List - Delete
// =============
#define SM_LIST_DELETE( list, entry ) \
    list = g_list_delete_link( list, entry );
// ****************************************************************************

// ****************************************************************************
// List - Cleanup
// ==============
#define SM_LIST_CLEANUP( list ) \
    g_list_free( list );        \
    list = NULL;
// ****************************************************************************

// ****************************************************************************
// List - Cleanup All
// ==================
#define SM_LIST_CLEANUP_ALL( list )                  \
    if( NULL != list )                               \
    {                                                \
        g_list_foreach( list, (GFunc)g_free, NULL ); \
        g_list_free( list );                         \
        list = NULL;                                 \
    }
// ****************************************************************************

// ****************************************************************************
// List - Cleanup All With Entry Cleanup
// =====================================
#define SM_LIST_CLEANUP_ALL_WITH_ENTRY_CLEANUP( list, cleanup_entry_callback ) \
    if( NULL != list )                                                         \
    {                                                                          \
        g_list_foreach( list, (GFunc)cleanup_entry_callback, NULL );           \
        g_list_free( list );                                                   \
        list = NULL;                                                           \
    }
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_LIST_H__
