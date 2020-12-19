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

extern uint8_t sid1;
extern uint8_t sid2;
uint8_t val = 0;

void* bread (){
   while (1)
   {  
      //printf ("A");
      pok_sem_signal (sid1);
   }
};

void* fired_chicken (){
   while (1)
   {  
      //printf ("B");
      pok_sem_signal (sid2);
   }
};

void* hamburger (){
   while (1)
   {  
      pok_sem_wait (sid1, 0);
      pok_sem_wait (sid2, 0);
      //printf ("C");
   }
};

void* soup (){
   while (1)
   {  
      //printf ("D");
   }
};

void* noodle (){
   while (1)
   {  
      //printf ("E");
   }
};