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
#include <core/semaphore.h>
#include <types.h>

extern uint8_t sid;
uint8_t val;

void* pinger_job ()
{
   // pok_ret_t ret;
   int count = 0;
   while (1)
   {
      if (count % 100000 == 0) //(count == 0)
      {
         // printf("P1T1: begin of task, count1= %d.\n", count);
         printf("P1T1: begin of task.\n");

      }
      count++;
      
      // printf ("P1T1: I will signal semaphores\n");
      // ret = pok_sem_signal (sid);
      // printf ("P1T1: pok_sem_signal, ret=%d\n", ret);
      // pok_thread_sleep (2000000);
   }
}

void* pinger_job2 ()
{
   // pok_ret_t ret;
   int count2 = 0;
   while (1)
   {
      if (count2 % 100000 == 0) //(count2 == 0)
      {
         // printf("P1T2: begin of task, count2= %d.\n", count2);
         printf("P1T2: begin of task.\n");
      }
      count2++;
      // printf("P1T2: begin of task\n");
   //    printf ("P1T2: I will wait for the semaphores\n");
   //    ret = pok_sem_wait (sid, 0);
   //    printf ("P1T2: pok_sem_wait, ret=%d\n", ret);
   //    ret = pok_sem_wait (sid, 0);
   //    printf ("P1T2: pok_sem_wait, ret=%d\n", ret);
   //    pok_thread_sleep (2000000);
   }
}

void* pinger_job3 () {

   int count3 = 0;
   while (1)
   {
      if (count3 % 100000 == 0)  //(count3 == 0)
      {
         // printf("P1T3: begin of task, count3= %d.\n", count3);
         printf("P1T3: begin of task.\n");
      }
      count3++;
   }
   
}