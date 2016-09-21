#ifndef OCUTILS_TIMER_H
#define OCUTILS_TIMER_H

struct __Timer;

typedef struct __Timer Timer;

struct __Timer {
  struct timespec begin;
  struct timespec end;
};

#define ocutils_timer_create() \
  calloc(sizeof(Timer),1)
#define ocutils_timer_start(TIMER) \
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&TIMER->begin)
#define ocutils_timer_stop(TIMER) \
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&TIMER->end)
#define ocutils_timer_get_usecs(TIMER) \
  ((TIMER->end.tv_sec - TIMER->begin.tv_sec)*1000000 + \
  (TIMER->end.tv_nsec - TIMER->begin.tv_nsec)/1000)
#endif
