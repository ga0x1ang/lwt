#ifndef LWT_H
#define LWT_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define STACK_SIZE      16 * 1024
#define unlikely(x)     __builtin_expect(!!(x), 0)

typedef enum {
        NEW,
        RUNNING,
        DEAD,
        BLOCKED,
        FINISHED,
        JOINED
} lwt_state;

typedef struct lwt {
        unsigned long id, sp, ip;
        lwt_state state;
        void *ret;
} *lwt_t; 

typedef struct lwt_node {
        lwt_t data;
        struct lwt_node *next;
} *lwt_node_t;

typedef struct lwt_queue {
        lwt_node_t head;
        lwt_node_t tail;
} *lwt_queue_t;

extern lwt_queue_t run_queue;
extern lwt_node_t active_thread;
unsigned long thd_counter;

void lwt_enqueue(lwt_node_t node);
lwt_node_t lwt_dequeue(void);

typedef void *(*lwt_fn_t)(void *);

lwt_t lwt_create(lwt_fn_t fn, void *data);
void __lwt_start(lwt_fn_t fn, void *data);
void __lwt_dispatch(lwt_t next, lwt_t current);
void lwt_die(void *ret);
lwt_node_t lwt_current(void);
void *lwt_join(lwt_t);
int lwt_yield(lwt_t);
void __lwt_schedule(void);

#endif
