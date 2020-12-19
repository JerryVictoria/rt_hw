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


#include <libc/stdio.h>
#include <core/thread.h>
#include <core/partition.h>
#include <core/semaphore.h>
#include <types.h>
#include "activity.h"

#define ONE_SCHED_TIME 9219000

int main ()
{
  uint8_t tid;
  pok_ret_t ret;
  pok_thread_attr_t     tattr;

  tattr.priority = 30;
  tattr.period = ONE_SCHED_TIME*4;
  tattr.time_capacity = 2;
  tattr.entry = job1;

  ret = pok_thread_create(&tid , &tattr);
  printf("[P1] pok_thread_create (job1) return=%d\n", ret);

  tattr.priority = 40;
  tattr.period = ONE_SCHED_TIME*4;
  tattr.time_capacity = 2;
  tattr.entry = job2;

  ret = pok_thread_create(&tid , &tattr);
  printf("[P1] pok_thread_create (job2) return=%d\n", ret);

  pok_partition_set_mode (POK_PARTITION_MODE_NORMAL);
  pok_thread_wait_infinite ();

   return (0);
}
