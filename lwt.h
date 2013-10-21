#ifndef LWT_H
#define LWT_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define STACK_SIZE      16 * 1024
#define LWT_NULL        NULL

typedef void *(*lwt_fn_t)(void *);
typedef void *lwt_ret; /* for aesthetic, look at the api function definitions at bottom */ 
extern unsigned long id_counter;
extern unsigned long n_runnable, n_zombies, n_blocked;

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
        unsigned long id, sp, ip;
        lwt_state state;
        void *ret;
        struct lwt *parent;
} *lwt_t; 

typedef struct lwt_node {
        lwt_t data;
        struct lwt_node *next;
} *lwt_node_t;
extern lwt_node_t active_thread;

typedef struct lwt_queue {
        lwt_node_t head;
        lwt_node_t tail;
} *lwt_queue_t;
extern lwt_queue_t run_queue;

/* run queue operation functions */ 
void       lwt_enqueue(lwt_node_t node);
lwt_node_t lwt_dequeue(void);

/* api functions */ 
lwt_t      lwt_create(lwt_fn_t fn, void *data);
int        lwt_yield(lwt_t);
lwt_ret    lwt_join(lwt_t);
void       lwt_die(void *ret);

inline unsigned long lwt_id(lwt_t thd);
inline lwt_node_t    lwt_current(void);
inline unsigned long lwt_info(lwt_info_t type);

#endif
