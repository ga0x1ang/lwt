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
        BLOCKED
} lwt_state;

typedef struct lwt {
        unsigned long id, sp, ip;
        lwt_state state;
} *lwt_t; 

typedef struct lwt_node {
        lwt_t data;
        struct lwt_node *next;
} *lwt_node_t;

typedef struct lwt_queue {
        lwt_node_t head;
        lwt_node_t tail;
} *lwt_queue_t;

static lwt_queue_t run_queue;

void lwt_enqueue(lwt_node_t node, lwt_queue_t q);
lwt_node_t lwt_dequeue(lwt_queue_t q);

typedef void *(*lwt_fn_t)(void *);

lwt_t lwt_create(lwt_fn_t fn, void *data);
void __lwt_start(int data);
void __lwt_dispatch(lwt_t next, lwt_t current);
#endif
