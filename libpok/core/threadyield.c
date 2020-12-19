/*
 *                               POK header
 * 
 * The following file is a part of the POK project. Any modification should
 * made according to the POK licence. You CANNOT use this file or a part of
 * this file is this part of a file for your own project
 *
 * For more information on the POK licence, please see our LICENCE FILE
 *
 * Please follow the coding guidelines described in doc/CODING_GUIDELINES
 *
 *                                      Copyright (c) 2007-2009 POK team 
 *
 * Created by julien on Thu Jan 15 23:34:13 2009 
 */

//#ifndef POK_CONFIG_OPTIMIZE_FOR_GENERATED_CODE

// created BY TJW

#ifdef POK_NEEDS_THREADS

#include <types.h>
#include <core/syscall.h>

void pok_thread_yield ()
{
   pok_syscall2 (POK_SYSCALL_THREAD_YIELD_AND_FULL, (uint32_t)NULL, (uint32_t)NULL);
}

#endif

//#endif /* POK_CONFIG_OPTIMIZE_FOR_GENERATED_CODE */

