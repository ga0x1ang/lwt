#ifndef LWT_H
#define LWT_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define STACK_SIZE      16 * 1024
#define LWT_NULL        NULL

extern volatile unsigned long id_counter;

typedef void *(*lwt_fn_t)(void *);
typedef void *lwt_ret;

typedef enum {
        LWT_INFO_NTHD_RUNNABLE,
        LWT_INFO_NTHD_BLOCKED,
        LWT_INFO_NTHD_ZOMBIES
} lwt_info_t;

typedef enum {
        RUNNABLE,
        BLOCKED,
        ZOMBIE
} lwt_state;

typedef struct lwt {
        void *mem_space;
        unsigned long id, sp;
        lwt_state state;
        struct lwt *parent;
        struct lwt *next;
        void *ret;
} __attribute__((aligned(4),packed)) *lwt_t; 

typedef struct lwt_queue {
        lwt_t head;
        lwt_t tail;
} __attribute__((aligned(4),packed)) *lwt_queue_t;
extern lwt_queue_t run_queue;


/* run queue operation functions */ 
inline void       lwt_enqueue(lwt_t node);
inline lwt_t lwt_dequeue(void);

/* api functions */ 
inline lwt_t      lwt_create(lwt_fn_t fn, void *data);
inline int        lwt_yield(lwt_t);
inline lwt_ret    lwt_join(lwt_t);
inline void       lwt_die(void *ret);
inline unsigned long lwt_id(lwt_t lwt);
inline unsigned long lwt_info(lwt_info_t type);
inline lwt_t lwt_current(void);

#endif
