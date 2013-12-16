#ifndef LWT_H
#define LWT_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <clist.h>
#include <pthread.h>

#define STACK_SIZE         (16*1024)
#define LWT_NULL           NULL
#define CHAN_SND_QUEUE_SZ  1024

extern volatile unsigned long id_counter;

typedef void *lwt_ret;

typedef enum {
        LWT_INFO_NTHD_RUNNABLE,
        LWT_INFO_NTHD_BLOCKED,
        LWT_INFO_NTHD_ZOMBIES,
        LWT_INFO_NTHD_NCHAN,
        LWT_INFO_NTHD_NSNDING,
        LWT_INFO_NTHD_NRCVING

} lwt_info_t;

typedef enum {
        RUNNABLE,
        BLOCKED,
        ZOMBIE
} lwt_state;

struct ktcb {
        pthread_t pthd;
        struct list_head *lwt_list;
};
typedef struct ktcb *ktcb_t;

#define LWT_NOJOIN         1
typedef enum {
        DEFAULT,
        NOJOIN 
} flags_t;

typedef struct lwt {
        void *mem_space;
        unsigned long id, sp;
        lwt_state state;
        struct lwt *parent;
        struct lwt *next;
        void *ret;
        flags_t flags;
        pthread_t kthd;
} *lwt_t; 

typedef struct lwt_queue {
        lwt_t head;
        lwt_t tail;
} *lwt_queue_t;

extern lwt_queue_t run_queue;

/* run queue operation functions */ 
inline void  lwt_enqueue(lwt_t node);
inline lwt_t lwt_dequeue(void);

/* api functions */ 
inline int        lwt_yield(lwt_t);
inline lwt_ret    lwt_join(lwt_t);
inline void       lwt_die(void *ret);
inline unsigned long lwt_id(lwt_t lwt);
inline unsigned long lwt_info(lwt_info_t type);
inline lwt_t lwt_current(void);

struct lwt_channel {
        /* sender's data */
        void *snd_data;
        int snd_cnt;  /* number of sending threads */
        struct clist_head *snd_thds;  /* list of those threads */

        /* receiver's data */
        void *mark_data; /* arbitrary value receiver */
        int rcv_blocked; /* if the receiver is blocked */ 
        lwt_t rcv_thd; /* the receiver */

        struct clist_head *msg_buf;
        
        struct lwt_cgrp *cgrp;
        struct lwt_channel *next;
        void *flag;
};
typedef struct lwt_channel *lwt_chan_t;
typedef void *(*lwt_fn_t)(void *data, lwt_chan_t c);

struct lwt_cgrp {
        lwt_t rcv_thd;
        lwt_chan_t head;
        lwt_chan_t tail;
        unsigned int chncnt;
};
typedef struct lwt_cgrp *lwt_cgrp_t;

lwt_chan_t lwt_chan(int sz);
void lwt_chan_deref(lwt_chan_t c);
int lwt_snd(lwt_chan_t c, void *data);
void *lwt_rcv(lwt_chan_t c);
void lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending);
lwt_chan_t lwt_rcv_chan(lwt_chan_t c);

typedef enum {
        LWT_CHAN_SND,
        LWT_CHAN_RCV
} lwt_chan_dir_t;

lwt_cgrp_t lwt_cgrp(void);
int lwt_cgrp_free(lwt_cgrp_t);
int lwt_cgrp_add(lwt_cgrp_t, lwt_chan_t, lwt_chan_dir_t direction);
int lwt_cgrp_rem(lwt_cgrp_t, lwt_chan_t);
lwt_chan_t lwt_cgrp_wait(lwt_cgrp_t, lwt_chan_dir_t *direction);
void lwt_chan_mark_set(lwt_chan_t, void *);
void *lwt_chan_mark_get(lwt_chan_t);
void lwt_chan_grant(lwt_chan_t);   /* TODO: USE TO PASS A CHAN TO CHILD (SIBLING COMMUNICATION) */ 

void lwt_snd_cdeleg(lwt_chan_t c, lwt_chan_t delegating);
lwt_chan_t lwt_rcv_cdeleg(lwt_chan_t c);
inline lwt_t      lwt_create(lwt_fn_t fn, void *data, flags_t flags, lwt_chan_t c);

int lwt_kthd_create(lwt_fn_t fn, void *data, lwt_chan_t c);

struct kthd_args {
        lwt_fn_t fn;
        void *data;
        lwt_chan_t c;
};
typedef struct kthd_args *kthd_args_t;

void kthd_snd(pthread_t *dest, void *data);
void *kthd_rcv(pthread_t *src);

//struct lwt_notification {
        //pthread_t from;
        //pthread_t to;
        //void *data;
//};

typedef enum kthd_status_t {
        NONE
} kthd_status_t;

typedef struct kthd {
        pthread_t *pthd;
        kthd_status_t status;
        struct kthd *prev;
        struct kthd *next;
} *kthd_t;

#endif
