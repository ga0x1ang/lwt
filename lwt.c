#include <lwt.h>
#include <clist.h>

/*#define likely(x)       __builtin_expect((x),1)*/
/*#define unlikely(x)     __builtin_expect((x),0)*/
#define likely(x)       x
#define unlikely(x)     x
#define printd          

#define gen_id()        ++id_counter

extern void __lwt_trampoline(void);

lwt_queue_t run_queue;
static volatile lwt_t active_thread;
volatile unsigned long id_counter;
static volatile unsigned long n_runnable, n_blocked, n_zombies, n_chans, n_sndings, n_rcvings;

__attribute__((constructor)) static void
lwt_init(void)
{
        id_counter = 0;
        n_runnable = 0, n_blocked = 0, n_zombies = 0;
        /* construct tcb */
        lwt_t master = malloc(sizeof(struct lwt));
        master->id = gen_id();
        master->state = RUNNABLE;
        master->next = NULL;
        master->parent = master;
        n_runnable++;

        /* init the run queue */
        run_queue = malloc(sizeof(struct lwt_queue));
        run_queue->head = run_queue->tail = NULL;
        active_thread = master;

        return;
}

static inline void
__lwt_dispatch(lwt_t next, lwt_t curr)
{
        __asm__ ("pushl $1f\n\t"
                 "pushal\n\t"
                 "movl %%esp, %0\n\t"
                 "movl %1, %%esp\n\t"
                 "popal\n\t"
                 "ret\n\t"
                 "1:\t"
                 : "=m" (curr->sp)
                 : "g" (next->sp)
                 : "esp");
        return;
}

static inline void
__lwt_schedule(void)
{
        if (run_queue->head == NULL) return;
        lwt_t next = lwt_dequeue();
        lwt_t curr = lwt_current();
        curr->next = NULL;
        if (curr->state != ZOMBIE) lwt_enqueue(curr);
        printd("schedule from thread %lu to thread %lu\n", curr->id, next->id);
        printd("curr->state is %d\n", curr->state);
        active_thread = next;
        __lwt_dispatch(next, curr);

        return;
}

inline void
lwt_enqueue(lwt_t node)
{
        if (unlikely(run_queue->tail == NULL)) run_queue->head = run_queue->tail = node;
        else {
                run_queue->tail->next = node;
                run_queue->tail = node;
        }
        return;
}

inline lwt_t 
lwt_dequeue()
{
        lwt_t node = NULL;
        if (likely(run_queue->head != NULL)) {
                node = run_queue->head;
                run_queue->head = node->next;
                if (unlikely(run_queue->tail == node)) run_queue->tail = NULL;
        }
        return node;
}


inline lwt_t
lwt_create(lwt_fn_t fn, void *data, flags_t flags)
{
        /* 1. allocate memory for thread */
        lwt_t thd = malloc(sizeof(struct lwt));

        /* 2. set up the stack */
        /* stack bottom: data, fn, _trampoline, 0, 0, 0, 0, sp, bp, 0, 0 */
        thd->id = gen_id();
        printd("thread %lu created thread %lu\n", lwt_current()->id, thd->id);
        thd->mem_space = malloc(STACK_SIZE);
        thd->sp = (unsigned long)thd->mem_space + STACK_SIZE;
        thd->ret = NULL;
        thd->state = RUNNABLE;
        n_runnable++;
        thd->parent = lwt_current();
        thd->next = NULL;
        thd->flags = flags;

        /**
         * save current esp and switch to the stack we are going to construct 
         */
        unsigned long current_sp, restored_sp;
        __asm__ __volatile__("movl %%esp, %0\n\t" /* save current esp */
                             "movl %2, %%esp\n\t" /* switch to new stack */
                             "pushl $__lwt_trampoline\n\t"
                             "movl %%esp, %1\n\t"
                             "pushl %3\n\t" /* eax and  ecx are used to store */
                             "pushl %4\n\t" /* args passed to __lwt_start */
                             "pushl $0\n\t"
                             "pushl $0\n\t"
                             "pushl %1\n\t"
                             "pushl %2\n\t"
                             "pushl $0\n\t"
                             "pushl $0\n\t"
                             "movl %%esp, %2\n\t"
                             "movl %0, %%esp"
                             : "=g" (current_sp), "=g" (restored_sp)
                             : "m" (thd->sp), "g" (fn), "g" (data)
                             : "esp");

        /* 3. add to run queue */
        lwt_enqueue(thd);

        return thd;
}

inline int
lwt_yield(lwt_t next)
{
        if (unlikely(next != LWT_NULL)) {
                while (run_queue->head!= next) {
                        lwt_t requeue = lwt_dequeue();
                        lwt_enqueue(requeue);
                }
                __lwt_schedule();
        } else if (likely(run_queue->head != NULL)) __lwt_schedule();

        return 0;
}

inline void *
lwt_join(lwt_t child)
{
        /*if (child->flags & LWT_NOJOIN) {*/
                /*assert(0);*/
                /*return NULL;*/
        /*}*/
        lwt_t curr = lwt_current();
        if (unlikely(child->parent != curr)) {
                printd("can only join by parent !!!\n");
                return NULL;
        }
        curr->state = BLOCKED;
        n_blocked++;
        n_runnable--;
        while (child->state != ZOMBIE) {
                printd("waiting for thread %lu to finish ...\n", child->id);
                __lwt_schedule();
        }
        printd("%lu joined %lu\n", lwt_current()->id, child->id);
        void *ret = child->ret;
        free(child->mem_space);
        free(child);

        curr->state = RUNNABLE;
        n_blocked--;
        n_runnable++;
        n_zombies--;

        return ret;
}

inline void
lwt_die(void *ret)
{
        lwt_t curr = lwt_current();
        printd("thread %lu died\n", curr->id);
        if (curr->flags & LWT_NOJOIN) return; /* TODO: how? */ 
        curr->ret = ret;

        curr->state = ZOMBIE;
        n_runnable--;
        n_zombies++;

        __lwt_schedule();

        return;
}

inline unsigned long
lwt_info(lwt_info_t type)
{
        unsigned long ret;
        switch (type)
        {
                case LWT_INFO_NTHD_RUNNABLE:
                        ret = n_runnable;
                        break;
                case LWT_INFO_NTHD_BLOCKED:
                        ret = n_blocked;
                        break;
                case LWT_INFO_NTHD_ZOMBIES:
                        ret = n_zombies;
                        break;
                case LWT_INFO_NTHD_NCHAN:
                        ret = n_chans;
                        break;
                case LWT_INFO_NTHD_NSNDING:
                        ret = n_sndings;
                        break;
                case LWT_INFO_NTHD_NRCVING:
                        ret = n_rcvings;
                        break;
                default:
                        ret = 0;
        }

        return ret;
}

inline unsigned long
lwt_id(lwt_t lwt)
{
        return lwt->id;
}

inline lwt_t
lwt_current(void)
{
        return active_thread;
}

lwt_chan_t
lwt_chan(int sz)
{
        lwt_chan_t c = malloc(sizeof(struct lwt_channel));
        c->snd_data = NULL;
        c->snd_cnt = 0;
        struct clist_head *cl = malloc(sizeof(struct clist_head));
        clist_init(cl, CHAN_SND_QUEUE_SZ);
        c->snd_thds = cl;
        c->mark_data = NULL;
        c->rcv_blocked = 0;
        c->rcv_thd = lwt_current();
        c->msg_buf = NULL;

        if(sz) {
                struct clist_head *msg_buf = malloc(sizeof(struct clist_head));
                clist_init(msg_buf, sz);
                c->msg_buf = msg_buf;
        }

        return c;
}

void
lwt_chan_deref(lwt_chan_t c)
{
        if (c->snd_cnt) c->snd_cnt--;
        if (c->snd_cnt == 0) clist_destroy(c->snd_thds);

        return;
}

int
lwt_snd(lwt_chan_t c, void *data)
{
        lwt_t curr = lwt_current();
                if (c->cgrp) {
                        if (!c->cgrp->tail)
                                c->cgrp->head = c->cgrp->tail = c;
                        // c->cgrp->head->next = c? tail?
                        else {
                                c->cgrp->tail->next = c;
                                c->cgrp->tail = c;
                        }
                        curr->state = BLOCKED;
                        lwt_yield(LWT_NULL);
                }
        if (c->msg_buf) {
                int buf_status = clist_add(c->msg_buf, data);
                while (buf_status < 0) {
                        printd("message buffer is full!\n");
                        curr->state = BLOCKED;
                        n_sndings++;
                        clist_add(c->snd_thds, (void *)curr);
                        c->rcv_thd->state = RUNNABLE;
                        lwt_yield(c->rcv_thd);
                        buf_status = clist_add(c->msg_buf, data);
                }
        } else {
                if (c->rcv_blocked) {
                        clist_add(c->snd_thds, (void *)curr);
                        n_sndings++;
                        curr->state = BLOCKED;
                        lwt_yield(LWT_NULL);
                }
                c->snd_data = data;
                c->rcv_thd->state = RUNNABLE;
                c->rcv_blocked = 1;
                lwt_yield(LWT_NULL);
        }
        n_sndings--;

        return 0; 
}

static inline void
__lwt_rcv_block(lwt_chan_t c)
{
        lwt_t curr = lwt_current();
        curr->state = BLOCKED;
        n_rcvings++;
        lwt_t next_sndr = (lwt_t)clist_get(c->snd_thds); /* is there queued snd_thds? */
        if (next_sndr) next_sndr->state = RUNNABLE;
        lwt_yield(LWT_NULL);
        n_rcvings--;

        return;
}


static inline void *
__lwt_rcv_get_data(lwt_chan_t c)
{
        void *ret = NULL;

        if (c->msg_buf) return clist_get(c->msg_buf);
        else {
                ret = c->snd_data;
                c->snd_data = NULL;
        }

        return ret;
}

void*
lwt_rcv(lwt_chan_t c)
{
        void *ret = NULL;
        ret = __lwt_rcv_get_data(c);
        while (!ret) {
                __lwt_rcv_block(c);
                ret = __lwt_rcv_get_data(c);
        }
        return ret;
}

lwt_chan_t
lwt_rcv_chan(lwt_chan_t c)
{
        lwt_chan_t rcv_chan = (lwt_chan_t)lwt_rcv(c);
        rcv_chan->snd_cnt++;
        assert(rcv_chan);
        printd("thread %lu received channel %p\n", lwt_id(lwt_current()), rcv_chan);
        return rcv_chan;
}

void
lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending)
{
        printd("thread %lu sent channel %p\n", lwt_id(lwt_current()), sending);
        lwt_snd(c, (void *)sending);
        return;
}

lwt_cgrp_t
lwt_cgrp(void)
{
        lwt_cgrp_t cgrp = malloc(sizeof(struct lwt_cgrp));
        cgrp->rcv_thd = lwt_current();
        cgrp->chncnt = 0;
        cgrp->head = cgrp->tail = NULL;

        return cgrp;
}

int
lwt_cgrp_free(lwt_cgrp_t cgrp)
{
        if (cgrp->chncnt) return -1;
        free(cgrp);
        return 0;
}

int
lwt_cgrp_add(lwt_cgrp_t cgrp, lwt_chan_t c)
{
        c->cgrp = cgrp;
        c->rcv_thd = cgrp->rcv_thd;
        if (!cgrp->head) cgrp->head = cgrp->tail = c;
        else {
                cgrp->tail->next = c;
                cgrp->tail = c;
        }
        cgrp->chncnt++;

        return 0;
}

int
lwt_cgrp_rem(lwt_cgrp_t cgrp, lwt_chan_t c)
{
        c->cgrp = NULL;
        c->rcv_thd = NULL;
        cgrp->chncnt--;

        return 0;
}

lwt_chan_t
lwt_cgrp_wait(lwt_cgrp_t cgrp)
{
        lwt_t curr = lwt_current();
        if (cgrp->rcv_thd != curr) return NULL;

        while (!cgrp->head) {
                curr->state = BLOCKED;
                lwt_yield(LWT_NULL);
        }

        lwt_chan_t ret = cgrp->head; /* TODO: pack this into functions */ 
        if (cgrp->head == cgrp->tail) cgrp->head = cgrp->tail = NULL; /* this sucks ... */ 
        else cgrp->head = cgrp->head->next;

        return ret;
}

void
lwt_chan_mark_set(lwt_chan_t c, void *flag)
{
        c->flag = flag;
        return;
}

void*
lwt_chan_mark_get(lwt_chan_t c)
{
        return c->flag;
}

void
lwt_chan_grant(lwt_chan_t c)
{
        return;
}
