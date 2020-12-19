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

uint8_t sid;

// int main ()
// {
//   uint8_t tid;
//   pok_ret_t ret;
//   pok_thread_attr_t     tattr;

//   ret = pok_sem_create(&sid , 0, 50, POK_SEMAPHORE_DISCIPLINE_FIFO);
//   printf("[P1] pok_sem_create return=%d, mid=%d\n", ret, sid);

//   tattr.priority = 50;
//   tattr.entry = pinger_job;

//   ret = pok_thread_create(&tid , &tattr);
//   printf("[P1] pok_thread_create (1) return=%d\n", ret);

//   tattr.priority = 40;
//   tattr.entry = pinger_job2;

//   ret = pok_thread_create(&tid , &tattr);
//   printf("[P1] pok_thread_create (2) return=%d\n", ret);

//   pok_partition_set_mode (POK_PARTITION_MODE_NORMAL);
//   pok_thread_wait_infinite ();

//    return (0);
// }
#define POK_CONFIG_NB_THREADS       8

pok_ret_t create_thread(int num){
  uint8_t tid;
  pok_ret_t ret;
  pok_thread_attr_t     tattr;

  switch (num)
  {
  case 0:
    tattr.priority = 50;
    tattr.entry = job0;
    tattr.weight = 1;
    ret = pok_thread_create(&tid , &tattr);
    printf("[P1] pok_thread_create (0) return=%d\n", ret);
    break;
  case 1:
    tattr.priority = 40;
    tattr.entry = job1;
    tattr.weight = 1;
    ret = pok_thread_create(&tid , &tattr);
    printf("[P1] pok_thread_create (1) return=%d\n", ret);
    break;
  case 2:
    tattr.priority = 30;
    tattr.weight = 2;
    tattr.entry = job2;
    ret = pok_thread_create(&tid , &tattr);
    printf("[P1] pok_thread_create (4) return=%d\n", ret);
    break;
  case 3:
    tattr.priority = 40;
    tattr.weight = 3;
    tattr.entry = job3;
    ret = pok_thread_create(&tid , &tattr);
    printf("[P1] pok_thread_create (3) return=%d\n", ret);
    break;
  default:
    printf("invalid input!\n");
    ret = -1;
    break;
  }
  return ret;
}


void* read_input(){
    int ret;
    int c = -1;
    int count = 0;
    printf("You can create %d threads at most.\n", POK_CONFIG_NB_THREADS - 4);
    while(count < POK_CONFIG_NB_THREADS - 4){
      printf("please choose a char from [0, 1, 2, 3]\n");
      while((c = getChar()) == EOF){
      }
      printf("get: %c\n", c);
      ret = create_thread(c-'0');
      if(ret == 0){
        count++;
      }
    }
    printf("You cannot create any thread now!\n");
}

int main ()
{
    uint8_t tid;
    pok_ret_t ret;
    pok_thread_attr_t     tattr;


    ret = pok_sem_create(&sid , 0, 50, POK_SEMAPHORE_DISCIPLINE_FIFO);
    printf("[P1] pok_sem_create return=%d, mid=%d\n", ret, sid);


    tattr.priority = 255;
    tattr.entry = read_input;
    tattr.weight = 10;
    ret = pok_thread_create(&tid , &tattr);
    printf("[in main] pok_thread_create (read_input) return=%d\n", ret);
    
    pok_partition_set_mode (POK_PARTITION_MODE_NORMAL);
    pok_thread_wait_infinite ();
    return (0);
}
