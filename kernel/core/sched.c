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

/**
 **\\file   sched.c
 **\\brief  Function for partitions and kernel scheduling
 **\\author Julien Delange
 */
#define MAX(a,b) ((a)>(b)) ? (a) : (b)
#if defined (POK_NEEDS_SCHED) || defined (POK_NEEDS_THREADS)

#include <types.h>
#include <arch.h>

#include <core/time.h>
#include <core/sched.h>
#include <core/thread.h>

#ifdef POK_NEEDS_PARTITIONS
#include <core/partition.h>
#endif

#ifdef POK_NEEDS_MIDDLEWARE
#include <middleware/port.h>
#endif

#include <dependencies.h>

#include <core/debug.h>
#include <core/instrumentation.h>
#include <core/error.h>

extern pok_thread_t       pok_threads[];

#ifdef POK_NEEDS_PARTITIONS
extern pok_partition_t    pok_partitions[];

/**
 * \brief The variable that contains the value of partition currently being executed
 */
uint8_t                   pok_current_partition;

void                      pok_sched_partition_switch();
#endif

#if defined (POK_NEEDS_PORTS_SAMPLING) || defined (POK_NEEDS_PORTS_QUEUEING)
extern void pok_port_flushall (void);
extern void pok_port_flush_partition (uint8_t);
#endif

uint64_t           pok_sched_slots[POK_CONFIG_SCHEDULING_NBSLOTS]
                              = (uint64_t[]) POK_CONFIG_SCHEDULING_SLOTS;
uint8_t           pok_sched_slots_allocation[POK_CONFIG_SCHEDULING_NBSLOTS]
                              = (uint8_t[]) POK_CONFIG_SCHEDULING_SLOTS_ALLOCATION;
                              
#ifdef DYNAMIC_PARTITIONS_PERCENT_MODE
uint8_t            pok_partition_percent[POK_CONFIG_NB_PARTITIONS]
                              = (uint8_t[]) DYNAMIC_PARTITIONS_PERCENTS;
#endif

pok_sched_t       pok_global_sched;
uint64_t          pok_sched_next_deadline;
uint64_t          pok_sched_next_major_frame;
uint64_t          pok_sched_next_flush; // variable used to handle user defined
                                        // flushing period, i.e. distinct from
                                        // MAF and from partition slot
                                        // boundaries
uint8_t           pok_sched_current_slot = 0; /* Which slot are we executing at this time ?*/
uint32_t	         current_thread = KERNEL_THREAD;

void pok_sched_thread_switch (void);

/**
 *\\brief Init scheduling service
 */

void mlfq_init(void);
void pok_sched_init (void)
{
#ifdef DYNAMIC_PARTITIONS_FIX_MODE
   int i;
   // example: POK_CONFIG_SCHEDULING_NBSLOTS = 4
   // {0, 1, 0, 0}
   #ifdef POK_NEEDS_SCHED_ZERO_MORE 
   {
      for(i = 0; i < POK_CONFIG_SCHEDULING_NBSLOTS; i++){
         pok_sched_slots_allocation[i] =  i % POK_CONFIG_NB_PARTITIONS;
         if(i >= POK_CONFIG_NB_PARTITIONS) pok_sched_slots_allocation[i] = 0;
      }
   }
   #endif
   // {0, 1, 1, 1}
   #ifdef POK_NEEDS_SCHED_FIRST_MORE
   {
      for(i = 0; i < POK_CONFIG_SCHEDULING_NBSLOTS; i++){
         pok_sched_slots_allocation[i] =  i % POK_CONFIG_NB_PARTITIONS;
         if(i >= POK_CONFIG_NB_PARTITIONS) pok_sched_slots_allocation[i] = 1;
      }
   }
   #endif
   // {0, 1, 0, 1} (default)
#else

   #ifdef DYNAMIC_PARTITIONS_PERCENT_MODE
      {
         int percent_index;
         int total = 0;
         for(percent_index = 0; percent_index < POK_CONFIG_NB_PARTITIONS; percent_index++){
            total += pok_partition_percent[percent_index];
         }
         uint8_t part_num[20], j, k, num;
         for(percent_index = 0; percent_index < POK_CONFIG_NB_PARTITIONS; percent_index++){
            num = pok_partition_percent[percent_index] * POK_CONFIG_SCHEDULING_NBSLOTS / total;
            part_num[percent_index] = num;
         }
         k = 0; // point to part_num_index
         for(j = 0; j < POK_CONFIG_SCHEDULING_NBSLOTS; j++) {
            while(part_num[k % POK_CONFIG_NB_PARTITIONS]==0) k++;
            pok_sched_slots_allocation[j] = k % POK_CONFIG_NB_PARTITIONS;
            part_num[k % POK_CONFIG_NB_PARTITIONS]--;
            k++;
         }

      }
   #else
      #ifndef POK_CONFIG_SCHEDULING_SLOTS_ALLOCATION
      int i;
      {
         for(i = 0; i < POK_CONFIG_SCHEDULING_NBSLOTS; i++){
            pok_sched_slots_allocation[i] =  i % POK_CONFIG_NB_PARTITIONS;
         }
      }
      #endif
   #endif

#endif
   int index = 0;
   printf("\npok_sched_slots_allocation: ");
   for(index = 0; index < POK_CONFIG_SCHEDULING_NBSLOTS; index++){
        printf("%d ", pok_sched_slots_allocation[index]);
   }
   printf("\n");
#ifdef POK_NEEDS_PARTITIONS 
#if defined (POK_NEEDS_ERROR_HANDLING) || defined (POK_NEEDS_DEBUG)
   /*
    * We check that the total time of time frame
    * corresponds to the sum of each slot
    */
   uint64_t                      total_time;
   uint8_t                       slot;

   total_time = 0;

   for (slot = 0 ; slot < POK_CONFIG_SCHEDULING_NBSLOTS ; slot++)
   {
      total_time = total_time + pok_sched_slots[slot];
   }

   if (total_time != POK_CONFIG_SCHEDULING_MAJOR_FRAME)
   {
#ifdef POK_NEEDS_DEBUG
      printf ("Major frame is not compliant with all time slots\n");
#endif
#ifdef POK_NEEDS_ERROR_HANDLING
      pok_kernel_error (POK_ERROR_KIND_KERNEL_CONFIG);
#endif
   }
#endif
#endif

   /* For MLFQ */
   mlfq_init(); 

   pok_sched_current_slot        = 0;
   pok_sched_next_major_frame    = POK_CONFIG_SCHEDULING_MAJOR_FRAME;
   pok_sched_next_deadline       = pok_sched_slots[0];
   pok_sched_next_flush          = 0;
   pok_current_partition         = pok_sched_slots_allocation[0];
}

uint8_t pok_sched_get_priority_min (const pok_sched_t sched_type)
{
   (void) sched_type;
   /* At this time, we only support one scheduler */
   return 0;
}

uint8_t pok_sched_get_priority_max (const pok_sched_t sched_type)
{
   (void) sched_type;
   /* At this time, we only support one scheduler */
   return 255;
}

#ifdef POK_NEEDS_PARTITIONS
uint8_t	pok_elect_partition()
{
  // printf("pok_elect_partition[a]\n");
  uint8_t next_partition = POK_SCHED_CURRENT_PARTITION;
# if POK_CONFIG_NB_PARTITIONS > 1
  uint64_t now = POK_GETTICK();

  if (pok_sched_next_deadline <= now)
  {
      /* Here, we change the partition */
#  if defined (POK_NEEDS_PORTS_SAMPLING) || defined (POK_NEEDS_PORTS_QUEUEING)
#    if defined (POK_FLUSH_PERIOD)
    // Flush periodically all partition ports
    // nb : Flush periodicity is a multiple of POK time base.
    printf("pok_elect_partition[b]\n");
    if (pok_sched_next_flush <= now)
    {
      pok_sched_next_flush += POK_FLUSH_PERIOD;
      pok_port_flushall();
    }
#    elif defined (POK_NEEDS_FLUSH_ON_WINDOWS)
    //Flush only the ports of the partition that just finished its slot
    printf("pok_elect_partition[c]\n");
    if ((pok_sched_next_deadline<=now))
    {
      pok_port_flush_partition (pok_current_partition);
    }
#    else // activate default flushing policy at each Major Frame beginning
   printf("pok_elect_partition[d]\n");
    if (pok_sched_next_major_frame <= now)
    {
      pok_sched_next_major_frame = pok_sched_next_major_frame +	POK_CONFIG_SCHEDULING_MAJOR_FRAME;
      pok_port_flushall();
    }
#    endif /* defined POK_FLUSH_PERIOD || POK_NEEDS_FLUSH_ON_WINDOWS */
#  endif /* defined (POK_NEEDS_PORTS....) */

    pok_sched_current_slot = (pok_sched_current_slot + 1) % POK_CONFIG_SCHEDULING_NBSLOTS;
    pok_sched_next_deadline = pok_sched_next_deadline + pok_sched_slots[pok_sched_current_slot];
/*
    *  FIXME : current debug session about exceptions-handled
      printf ("Switch from partition %d to partition %d\n", pok_current_partition, pok_sched_current_slot);
      printf ("old current thread = %d\n", POK_SCHED_CURRENT_THREAD);

      printf ("new current thread = %d\n", pok_partitions[pok_sched_current_slot].current_thread);
      printf ("new prev current thread = %d\n", pok_partitions[pok_sched_current_slot].prev_thread);
      */
    next_partition = pok_sched_slots_allocation[pok_sched_current_slot];

#ifdef POK_NEEDS_SCHED_HFPPS
   if (pok_partitions[next_partition].payback > 0) // pay back!
   {
     // new deadline
     pok_sched_next_deadline -= pok_partitions[next_partition].payback;
     pok_partitions[next_partition].payback = 0;
   }
#endif /* POK_NEEDS_SCHED_HFPPS */

  }
# endif /* POK_CONFIG_NB_PARTITIONS > 1 */

  return next_partition;
}
#endif /* POK_NEEDS_PARTITIONS */

#ifdef POK_NEEDS_PARTITIONS
uint32_t	pok_elect_thread(uint8_t new_partition_id)
{
   //printf("in pok_elect_thread, new_partition_id:%d\n", new_partition_id);
   uint64_t now = POK_GETTICK();
   pok_partition_t* new_partition = &(pok_partitions[new_partition_id]);


   /*
    * We unlock all WAITING threads if the waiting time is passed
    */
   uint8_t i;           /* i is used to browse the partition. We support
                         * only 255 partitions are max, so, we use an uin8_t
                         * type
                         */
   pok_thread_t* thread;
   for (i = 0; i < new_partition->nthreads; i++)
   {
     thread = &(pok_threads[new_partition->thread_index_low + i]);
     //printf("index: %d. prio:%d, thread->state: %d, wakeup: %u, now: %u\n",new_partition->thread_index_low + i, thread->priority, thread->state, thread->wakeup_time, now);
#if defined (POK_NEEDS_LOCKOBJECTS) || defined (POK_NEEDS_PORTS_QUEUEING) || defined (POK_NEEDS_PORTS_SAMPLING)
     if ((thread->state == POK_STATE_WAITING) && (thread->wakeup_time <= now))
     {
       thread->state = POK_STATE_RUNNABLE;
   //   printf("runnable: %d\n", new_partition->thread_index_low + i);
     }
   //   else if((thread->state == POK_STATE_WAITING)){
   //      printf("now: %u\n", now);
   //      printf("thread->wakeup_time : %u\n", thread->wakeup_time );
   //     printf("thread->wakeup_time > now: %d\n", new_partition->thread_index_low + i);
   //   }
#endif
      // if(thread->state == POK_STATE_WAIT_NEXT_ACTIVATION){
      //    printf("%d, next: %d\n",new_partition->thread_index_low + i, thread->next_activation);
      // }
     if ((thread->state == POK_STATE_WAIT_NEXT_ACTIVATION) && (thread->next_activation <= now))
     {
       thread->state = POK_STATE_RUNNABLE;
       thread->remaining_time_capacity =  thread->time_capacity;
#ifdef POK_NEEDS_SCHED_EDF
       thread->edf_end_time = thread->next_activation + thread->deadline;
#endif
       thread->next_activation = thread->next_activation + thread->period; 
       // printf("%d, thread->remaining_time_capacity:%d\n",new_partition->thread_index_low + i, thread->remaining_time_capacity);
     }
   }

   /*
    * We elect the thread to be executed.
    */
   uint32_t elected;
   switch (new_partition->mode)
   {
      case POK_PARTITION_MODE_INIT_COLD:
      case POK_PARTITION_MODE_INIT_WARM:
#ifdef POK_NEEDS_ERROR_HANDLING
         if ((new_partition->thread_error != 0) &&
             (pok_threads[new_partition->thread_error].state != POK_STATE_STOPPED))
         {
            elected = new_partition->thread_error;
         }
         else
         {
            elected = new_partition->thread_main;
         }
#endif

         elected = new_partition->thread_main;
         break;

      case POK_PARTITION_MODE_NORMAL:
         // printf("POK_PARTITION_MODE_NORMAL\n");
#ifdef POK_NEEDS_ERROR_HANDLING
         if ((new_partition->current_thread == new_partition->thread_error) && 
             (pok_threads[new_partition->current_thread].state == POK_STATE_RUNNABLE))
         {
            elected = new_partition->thread_error;
            break;
         }
#endif
         if ( (POK_SCHED_CURRENT_THREAD != IDLE_THREAD) && 
              (POK_SCHED_CURRENT_THREAD != POK_CURRENT_PARTITION.thread_main) 
#ifdef POK_NEEDS_ERROR_HANDLING
              && (POK_SCHED_CURRENT_THREAD != POK_CURRENT_PARTITION.thread_error)
#endif
            )
         {
         //PRIO/WRR do not need budget change
         //#ifndef POK_NEEDS_SCHED_PRIO 
         #ifndef POK_NEEDS_SCHED_WRR
            //printf("%d, %d\n", POK_SCHED_CURRENT_THREAD, POK_CURRENT_THREAD.remaining_time_capacity);
            if (POK_CURRENT_THREAD.remaining_time_capacity > 0)
            {
               // printf("%d remaining_time_capacity : %d\n", POK_SCHED_CURRENT_THREAD, POK_CURRENT_THREAD.remaining_time_capacity);
               POK_CURRENT_THREAD.remaining_time_capacity = POK_CURRENT_THREAD.remaining_time_capacity - 1;
            }
            if (POK_CURRENT_THREAD.remaining_time_capacity <= 0){
               //printf("in POK_CURRENT_THREAD.time_capacity: %d\n", POK_CURRENT_THREAD.time_capacity);
               if(POK_CURRENT_THREAD.time_capacity > 0) // Wait next activation only for thread 
                                                            // with non-infinite capacity (could be 
                                                            // infinite with value -1 <--> INFINITE_TIME_CAPACITY)
               {
                  //printf("in POK_STATE_WAIT_NEXT_ACTIVATION\n");
                  POK_CURRENT_THREAD.state = POK_STATE_WAIT_NEXT_ACTIVATION;
               }
            }
         #endif
         //#endif
         }
         elected = new_partition->sched_func (new_partition->thread_index_low,
                                                     new_partition->thread_index_high,
						     new_partition->prev_thread,
						     new_partition->current_thread);
#ifdef POK_NEEDS_INSTRUMENTATION
          if ( (elected != IDLE_THREAD) && (elected != new_partition->thread_main))
          {
            pok_instrumentation_running_task (elected);
          }
#endif

         break;

      default:
         elected = IDLE_THREAD;
         break;
   }

#ifdef POK_NEEDS_SCHED_HFPPS
   if (pok_threads[elected].payback > 0) // pay back!
   {
     pok_threads[elected].remaining_time_capacity -= pok_threads[elected].payback;
     pok_threads[elected].payback = 0;
   }
#endif /* POK_NEEDS_SCHED_HFPPS */

   // computed next thread's deadline
   pok_threads[elected].end_time = now + pok_threads[elected].remaining_time_capacity;

   return (elected);
}
#endif /* POK_NEEDS_PARTITIONS */

#ifdef POK_NEEDS_PARTITIONS
void pok_sched()
{

  // printf("pok_sched\n");
  uint32_t elected_thread = 0;
  uint8_t elected_partition = POK_SCHED_CURRENT_PARTITION;

#ifdef POK_NEEDS_SCHED_MLFQ
	/* if job is not finished */
	if (pok_threads[current_thread].remaining_time_capacity > 0){
		pok_threads[current_thread].mlfq_before_level = MLFQ_QUEUE_LEVEL;
	}
#endif

#ifdef POK_NEEDS_SCHED_HFPPS
  uint64_t now = POK_GETTICK();
  elected_thread = current_thread;

  /* if thread hasn't finished its job and its deadline is passed */
  if (pok_threads[elected_thread].end_time <= now && pok_threads[elected_thread].remaining_time_capacity > 0)
  {
    /* updates thread and partition payback */
    pok_threads[elected_thread].payback = pok_threads[elected_thread].remaining_time_capacity;
    pok_partitions[pok_current_partition].payback = pok_threads[elected_thread].remaining_time_capacity;
    /* computes next partition deadline */
    pok_sched_next_deadline += pok_threads[elected_thread].remaining_time_capacity;
  }
  else /* overmegadirty */
#endif /* POK_NEEDS_SCHED_HFPPS */
  {
    elected_partition = pok_elect_partition();
    elected_thread = pok_elect_thread(elected_partition);
  }

   pok_current_partition = elected_partition;
   if(pok_partitions[pok_current_partition].current_thread != elected_thread) {
	   if(pok_partitions[pok_current_partition].current_thread != IDLE_THREAD) {
		   pok_partitions[pok_current_partition].prev_thread = pok_partitions[pok_current_partition].current_thread;
	   }
	   pok_partitions[pok_current_partition].current_thread = elected_thread;
   }

   if(pok_partitions[pok_current_partition].mode == POK_PARTITION_MODE_NORMAL){
      uint64_t time = POK_GETTICK();
      uint64_t theory_time = POK_GETTICK() / 9219000;
      printf("time: %u, ", time);
      printf("now: %u, ", theory_time);
      switch (elected_thread)
      {
         case POK_CONFIG_NB_THREADS-1:
            printf("IDLE THREAD\n");
            break;

         default:
            printf("%c\n", 'A' + elected_thread - 1);
            break;
      }
      // printf("elect: %c\n", 'A' + elected_thread - 1); // bugs...
   }
   
  pok_sched_context_switch(elected_thread);
}
#else
void pok_sched_thread_switch ()
{
   int i;
   uint64_t now;
   uint32_t elected;

   now = POK_GETTICK();
   for (i = 0; i <= POK_CONFIG_NB_THREADS; ++i)
   {
     if ((pok_threads[i].state == POK_STATE_WAITING) &&
	 (pok_threads[i].wakeup_time <= now))
      {
         pok_threads[i].state = POK_STATE_RUNNABLE;
      }
   }

   elected = pok_sched_part_election (0, POK_CONFIG_NB_THREADS);
   /*
    *  FIXME : current debug session about exceptions-handled
   printf ("switch to thread %d\n", elected);
   */
   pok_sched_context_switch(elected);
}
#endif /* POK_NEEDS_PARTITIONS */

/*
 * Context-switch function to switch from one thread to another
 * Rely on architecture-dependent functionnalities (must include arch.h)
 */
void pok_sched_context_switch (const uint32_t elected_id)
{
   // printf("pok_sched_context_switch\n");
   uint32_t *current_sp;
   uint32_t new_sp;

   if (POK_SCHED_CURRENT_THREAD == elected_id)
   {
      return;
   }

   current_sp = &POK_CURRENT_THREAD.sp;
   new_sp = pok_threads[elected_id].sp;
/*
    *  FIXME : current debug session about exceptions-handled
   printf("switch from thread %d, sp=0x%x\n",POK_SCHED_CURRENT_THREAD, current_sp);
   printf("switch to thread %d, sp=0x%x\n",elected_id, new_sp);
   */
   pok_space_switch(POK_CURRENT_THREAD.partition,
		    pok_threads[elected_id].partition);

   current_thread = elected_id;

   pok_context_switch(current_sp, new_sp);
}

#ifdef POK_NEEDS_SCHED_RMS
uint32_t pok_sched_part_rms (const uint32_t index_low, const uint32_t index_high,const uint32_t __attribute__((unused)) prev_thread,const uint32_t __attribute__((unused)) current_thread)
{
   printf("pok_sched_part_rms\n");
   uint32_t res;
#ifdef POK_NEEDS_DEBUG
   uint32_t from;
   from = prev_thread;
#endif

   res= index_low;

   do
   {
      res++;
      if (res >= index_high)
      {
         res = index_low;
      }
   }
   while ((res != index_low) &&
	  (pok_threads[res].state != POK_STATE_RUNNABLE));

   if ((res == index_low) && (pok_threads[res].state != POK_STATE_RUNNABLE))
   {
      res = IDLE_THREAD;
   }

#ifdef POK_NEEDS_DEBUG
    if ( res!= IDLE_THREAD)
    {
        printf("--- scheduling thread: %d {%d} --- ", res,
	       pok_threads[res].period);
        from=index_low;
        while ( from <= index_high )
        {
            if ( pok_threads[from].state==POK_STATE_RUNNABLE )
            {
                printf(" %d {%d} ,",from,pok_threads[from].period);
            }
            from++;
        }
        printf(" are runnable; \n\t\t");
        from=index_low;	
	while ( from <= index_high )
        {
            if ( pok_threads[from].state!=POK_STATE_RUNNABLE )
            {
                printf(" %d (state = %d)",from,pok_threads[from].state);
            }
            from++;
        }
        printf(" are NOT runnable;\n");

    }
#endif

   return res;
}
#endif /* POK_NEEDS_SCHED_RMS */

/*private methods served for pok_sched_part_wrr*/
uint32_t gcd_of_two(uint32_t x, uint32_t y){
   // printf("gcd_of_two\n");
   while(1){
      if(x == 0 || y == 0) {
         return 1;
      }
      if(x > y) x -= y;
      else if(x < y) y -= x;
      else return x;
   }
}
uint32_t wgcd_of_threads(const uint32_t index_low, const uint32_t index_high){
   uint32_t res = pok_threads[index_low].weight;
   uint32_t i;
   for(i = index_low + 1; i <= index_high; i++){
      res = gcd_of_two(res, pok_threads[i].weight);
      if(res == 1) return 1;
   }
   return res;
}
uint32_t wmax_of_thread(const uint32_t index_low, const uint32_t index_high){
   uint32_t res = pok_threads[index_low].weight;
   uint32_t i;
   for(i = index_low + 1; i <= index_high; i++){
      res = MAX(res, pok_threads[i].weight);
   }
   return res;
}

/* MLFQ sched algorithm */
uint32_t mlfq_queue_list[MLFQ_QUEUE_LEVEL][MLFQ_QUEUE_SIZE];
uint32_t head[MLFQ_QUEUE_LEVEL];
uint32_t tail[MLFQ_QUEUE_LEVEL];
uint64_t mlfq_boost_timestamp;
void mlfq_enqueue(uint32_t level, uint32_t thread)
{
	/* Check if full */
	uint32_t curr_head = head[level];
	uint32_t curr_tail = tail[level];
	if ((curr_tail + 1) % MLFQ_QUEUE_SIZE == curr_head){
		printf("[%s]: mlfq queue is full.\n", __func__);
		while(1);
	}
	/* Do enqueue */
	mlfq_queue_list[level][curr_tail] = thread;
	curr_tail = (curr_tail + 1) % MLFQ_QUEUE_SIZE;
	tail[level] = curr_tail;
}

uint32_t mlfq_dequeue(uint32_t level)
{
	/* Check if empty */
	uint32_t curr_head = head[level];
	uint32_t curr_tail = tail[level];
	if (curr_head == curr_tail){
		return IDLE_THREAD;
	}
	/* Do dequeue */
	uint32_t res_thread = mlfq_queue_list[level][curr_head]; 
	curr_head = (curr_head + 1) % MLFQ_QUEUE_SIZE;
	head[level] = curr_head;
	return res_thread;
}
void mlfq_init(void)
{
	for(uint32_t lv = 0; lv < MLFQ_QUEUE_LEVEL; lv++){
		head[lv] = 0;
		tail[lv] = 0;
		for(uint32_t i = 0; i < MLFQ_QUEUE_SIZE; i++){
			mlfq_queue_list[lv][i] = IDLE_THREAD; 
		}
	}
	mlfq_boost_timestamp = 0;
	printf("[%s]: Done.\n", __func__);
}

void mlfq_boost(void)
{
	for(uint32_t lv = 1; lv < MLFQ_QUEUE_LEVEL; lv++){
		uint32_t thread = mlfq_dequeue(lv);	
		if(thread != IDLE_THREAD){
			mlfq_enqueue(0,thread);
			pok_threads[thread].mlfq_before_level = MLFQ_QUEUE_LEVEL;
		}
	}
}
uint32_t pok_sched_part_mlfq (const uint32_t index_low, const uint32_t index_high,
		const uint32_t __attribute__((unused))prev_thread,
		const uint32_t __attribute__((unused)) current_thread)
{
   	/* Part 1: scan the whole array to add threads into mlfq_queue */
   	for(uint32_t idx = index_low; idx <= index_high; idx++){
		if(pok_threads[idx].state != POK_STATE_RUNNABLE){
			continue;
		}
		if(pok_threads[idx].if_inqueue != 0){
			continue;
		}
		/* Decide the next level */
		uint32_t next_level = 0;
		if(pok_threads[idx].mlfq_before_level != MLFQ_QUEUE_LEVEL){
			next_level = (pok_threads[idx].mlfq_before_level + 1) 
					% MLFQ_QUEUE_LEVEL;
		}
		/* Do the modification */
		pok_threads[idx].if_inqueue = 1;
		pok_threads[idx].mlfq_before_level = next_level;
		mlfq_enqueue(next_level, idx);
   	}

	/* Part 2: Pick next thread from the mlfq queue list */
	uint32_t next_thread = IDLE_THREAD;
	for(uint32_t lv = 0; lv < MLFQ_QUEUE_LEVEL; lv++){
		uint32_t thread = mlfq_dequeue(lv);	
		if(thread != IDLE_THREAD){
			pok_threads[thread].if_inqueue = 0;
			pok_threads[thread].mlfq_before_level = lv;	
			next_thread = thread;
			break;
		}
	}

	/* Part 3: Boost */
	uint64_t theory_time = POK_GETTICK() / 9219000;
	if(theory_time - mlfq_boost_timestamp >= MLFQ_BOOST_GAP){
		mlfq_boost_timestamp = theory_time;
		mlfq_boost();
	}	
	
	if (next_thread != IDLE_THREAD){
		return next_thread;
	}

	/* retur IDLE if no thread to sched */
	return IDLE_THREAD;	
}

uint32_t pok_sched_part_wrr (const uint32_t index_low, const uint32_t index_high,const uint32_t prev_thread,const uint32_t current_thread)
{
   // printf("pok_sched_part_wrr, index_low:%d, index_high:%d\n", index_low, index_high);
   uint32_t res;

   if (current_thread == IDLE_THREAD)
   {
      res = prev_thread;
   }
   else
   {
      res = current_thread;
   }

   // we need to judge whether all threads(except idle) are not in POK_STATE_RUNNABLE
   // if so, return IDLE_THREAD
   int total = index_high - index_low + 1;
   int count = 0;
   int runnable_existed = 0;
   static int cw = 0;
   // we don't need the main_thread
   // only at the first time, res == 0
   // so we initialze cw here only once
   if(res == 0){
      cw = wmax_of_thread(index_low, index_high);
   }
   while(1)
   {
      res++;
      count++;
      if (res > index_high)
      {
         res = index_low;
         cw = cw - wgcd_of_threads(index_low, index_high);
         if(cw <= 0) {
            cw = wmax_of_thread(index_low, index_high);
            if(cw == 0) {
               // printf("choose[a]: %d, cw: %d    ", IDLE_THREAD, cw);
               return IDLE_THREAD;
            }
         }
      }
      // if(pok_threads[res].weight >= cw && pok_threads[res].state != POK_STATE_RUNNABLE)
      // {
      //    printf(" res:%d is not runnable. ", res);
      // }
      if(pok_threads[res].state == POK_STATE_RUNNABLE){
         runnable_existed = 1;
         if(pok_threads[res].weight >= cw) {
            // printf("choose[b]: %d, cw: %d    ", IDLE_THREAD, cw);
            return res;
         }
      }
      if(count == total && !runnable_existed) {
         break;
      }
   }

   res = IDLE_THREAD;
   
   // printf("choose[c]: %d, cw: %d    ", IDLE_THREAD, cw);

   return res;

}

uint32_t pok_sched_part_prio (const uint32_t index_low, const uint32_t index_high,const uint32_t prev_thread,const uint32_t current_thread)
{
   // printf("pok_sched_part_prio, index_low: %d, index_high: %d\n", index_low, index_high);
   uint32_t res;
   uint32_t from;
   uint32_t selected;
   int selected_flag = 0;

   if (current_thread == IDLE_THREAD)
   {
      res = prev_thread;
   }
   else
   {
      res = current_thread;
   }

   from = res;

   // select a highest prio
   int highest_prio = -1;
   
   do
   {
      res++;
      if (res > index_high)
      {
         res = index_low;
      }
      // if(pok_threads[res].priority > highest_prio){
      //    printf("res: %d, %d, %d\n", res, pok_threads[res].state, pok_threads[res].remaining_time_capacity);
      // }
      if(pok_threads[res].priority > highest_prio && pok_threads[res].state == POK_STATE_RUNNABLE && pok_threads[res].remaining_time_capacity > 0){
         highest_prio = pok_threads[res].priority;
         selected = res;
         selected_flag = 1;
      }
   }
   while (res != from);

   if (selected_flag == 0)
   {
      res = IDLE_THREAD;
   }
   else
   {
      //printf("selected new prio thread\n");
      if(pok_threads[from].state == POK_STATE_RUNNABLE && pok_threads[from].priority >= pok_threads[selected].priority  && pok_threads[res].remaining_time_capacity > 0){
         selected = from;
      }
      res = selected;
      // if(pok_threads[res].remaining_time_capacity == 0){
      //    pok_threads[res].state = POK_STATE_WAIT_NEXT_ACTIVATION;
      // }
   }
   //printf("select %u\n", res);
   
   return res;

}
uint32_t pok_sched_part_edf (const uint32_t index_low, const uint32_t index_high,const uint32_t __attribute__((unused)) prev_thread,const uint32_t current_thread) {
   // printf("------------------------------------------------------------------------------\n");
   // printf("pok_sched_part_edf, index_low:%d, index_high:%d, current_thread:%d\n", index_low, index_high,current_thread);
   uint32_t res = index_low;
   uint32_t from = res;

   int flag = 0;
   uint32_t selected;
   uint64_t earliest_ddl = 0;
   if (pok_threads[current_thread].state == POK_STATE_RUNNABLE)
   {
      earliest_ddl = pok_threads[current_thread].edf_end_time;
   }
   do
   {
      res++;
      if (res > index_high)
      {
         res = index_low;
      }
      if ((earliest_ddl == 0 || pok_threads[res].edf_end_time <= earliest_ddl) 
         && pok_threads[res].remaining_time_capacity > 0 && pok_threads[res].state == POK_STATE_RUNNABLE){
         earliest_ddl = pok_threads[res].edf_end_time;
         selected = res;
         flag = 1;
         // printf("earliest_ddl = %d\n",earliest_ddl);
      }
   } while (res != from);

   if (flag == 0) {
      res = IDLE_THREAD;
   } 
   else
   {
      res = selected;
      // printf("[SCHED THREAD] Selected Thread ID = %d\n", res);
      // printf("[SCHED THREAD] Selected Thread DDL = %d\n", pok_threads[res].deadline);
      // printf("[SCHED THREAD] Selected Thread edf_end_time = %d\n", pok_threads[res].edf_end_time);
      // printf("[SCHED THREAD] Selected Thread period = %d\n", pok_threads[res].period);
      // printf("[SCHED THREAD] Selected Thread next_activation = %d\n", pok_threads[res].next_activation);
   }
   // printf("[EDF sche result] res = %u\n", res);
   // printf("------------------------------------------------------------------------------\n");
   return res;
}

uint32_t pok_sched_part_rr (const uint32_t index_low, const uint32_t index_high,const uint32_t prev_thread,const uint32_t current_thread)
{
   // printf("in pok_sched_part_rr\n");
   uint32_t res;
   uint32_t from;

   if (current_thread == IDLE_THREAD)
   {
      res = prev_thread;
   }
   else
   {
      res = current_thread;
   }

   from = res;

   if ((pok_threads[current_thread].remaining_time_capacity > 0) && (pok_threads[current_thread].state == POK_STATE_RUNNABLE))
   {
      return current_thread;
   }

   do
   {
      res++;
      if (res > index_high)
      {
         res = index_low;
      }
   }
   while ((res != from) && (pok_threads[res].state != POK_STATE_RUNNABLE));

   if ((res == from) && (pok_threads[res].state != POK_STATE_RUNNABLE))
   {
      res = IDLE_THREAD;
   }
   return res;
}


#if defined (POK_NEEDS_LOCKOBJECTS) || defined (POK_NEEDS_PORTS_QUEUEING) || defined (POK_NEEDS_PORTS_SAMPLING)
void pok_sched_unlock_thread (const uint32_t thread_id)
{
   pok_threads[thread_id].state = POK_STATE_RUNNABLE;
}
#endif

#if defined (POK_NEEDS_LOCKOBJECTS) || defined (POK_NEEDS_PORTS_QUEUEING) || defined (POK_NEEDS_PORTS_SAMPLING)
void pok_sched_lock_current_thread (void)
{
   pok_threads[current_thread].state = POK_STATE_LOCK;
}

void pok_sched_lock_current_thread_timed (const uint64_t time)
{
   pok_threads[current_thread].state = POK_STATE_WAITING;
   pok_threads[current_thread].wakeup_time = time;
}
#endif

#ifdef POK_NEEDS_SCHED_STOP_SELF
void pok_sched_stop_self (void)
{
   POK_CURRENT_THREAD.state = POK_STATE_STOPPED;
   pok_sched ();
}
#endif

void pok_sched_stop_thread (const uint32_t tid)
{
   pok_threads[tid].state = POK_STATE_STOPPED;
}

#ifdef POK_NEEDS_DEPRECIATED
void pok_sched_lock_thread (const uint32_t thread_id)
{
   pok_threads[thread_id].state = POK_STATE_LOCK;
}
#endif

pok_ret_t pok_sched_end_period ()
{
   POK_CURRENT_THREAD.state = POK_STATE_WAIT_NEXT_ACTIVATION;
   POK_CURRENT_THREAD.remaining_time_capacity = 0;
   pok_sched ();
   return POK_ERRNO_OK;
}

#if defined (POK_NEEDS_PARTITIONS) && defined (POK_NEEDS_ERROR_HANDLING)
void pok_sched_activate_error_thread (void)
{
   uint32_t error_thread = pok_partitions[pok_current_partition].thread_error;
   if (error_thread != 0)
   {
      pok_threads[error_thread].priority = pok_sched_get_priority_max(0);
      pok_threads[error_thread].remaining_time_capacity = 1000;
      pok_threads[error_thread].period = 100;
      pok_threads[error_thread].next_activation= 0;

      pok_threads[error_thread].state  = POK_STATE_RUNNABLE;
      pok_sched_context_switch (error_thread);
   }
}
#endif

#ifdef POK_NEEDS_PARTITIONS

uint32_t pok_sched_get_current(uint32_t *thread_id)
{
#if defined (POK_NEEDS_ERROR_HANDLING)
  if (pok_partitions[pok_current_partition].thread_error == 0)
    return POK_ERRNO_THREAD;
#endif
  if (KERNEL_THREAD == POK_SCHED_CURRENT_THREAD 
      || IDLE_THREAD == POK_SCHED_CURRENT_THREAD)
    {
      return POK_ERRNO_THREAD;
    }
  *thread_id=POK_SCHED_CURRENT_THREAD;
  return POK_ERRNO_OK;
}
#endif

#endif /* __POK_NEEDS_SCHED */
