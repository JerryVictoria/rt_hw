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
uint8_t sid1;
uint8_t sid2;
int main ()
{
  uint8_t tid;
  pok_ret_t ret;
  pok_thread_attr_t     tattr;

  ret = pok_sem_create(&sid1 , 0, 50, POK_SEMAPHORE_DISCIPLINE_FIFO);
  printf("[P1] pok_sem_create return=%d, mid=%d\n", ret, sid1);

  ret = pok_sem_create(&sid2 , 0, 50, POK_SEMAPHORE_DISCIPLINE_FIFO);
  printf("[P1] pok_sem_create return=%d, mid=%d\n", ret, sid1);

  tattr.weight = 5;
  tattr.priority = 10;
  tattr.period = 9 * ONE_SCHED_TIME;
  tattr.time_capacity = 1;
  tattr.entry = bread;

  ret = pok_thread_create(&tid , &tattr);
  printf("[P1] pok_thread_create (bread) return=%d\n", ret);

  tattr.weight = 5;
  tattr.priority = 10;
  tattr.period = 9 * ONE_SCHED_TIME;
  tattr.time_capacity = 2;
  tattr.entry = fired_chicken;

  ret = pok_thread_create(&tid , &tattr);
  printf("[P1] pok_thread_create (fired_chicken) return=%d\n", ret);

  tattr.weight = 3;
  tattr.priority = 5;
  tattr.period = 8 * ONE_SCHED_TIME;
  tattr.time_capacity = 1;
  tattr.entry = hamburger;

  ret = pok_thread_create(&tid , &tattr);
  printf("[P1] pok_thread_create (hamburger) return=%d\n", ret);

  tattr.weight = 2;
  tattr.priority = 6;
  tattr.period = 8 * ONE_SCHED_TIME;
  tattr.time_capacity = 2;
  tattr.entry = soup;

  ret = pok_thread_create(&tid , &tattr);
  printf("[P1] pok_thread_create (soup) return=%d\n", ret);

  tattr.weight = 4;
  tattr.priority = 7;
  tattr.period = 5 * ONE_SCHED_TIME;
  tattr.time_capacity = 1;
  tattr.entry = noodle;

  ret = pok_thread_create(&tid , &tattr);
  printf("[P1] pok_thread_create (noodle) return=%d\n", ret);

  pok_partition_set_mode (POK_PARTITION_MODE_NORMAL);
  pok_thread_wait_infinite ();

   return (0);
}
