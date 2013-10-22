#include "lwt.h"

/*#define likely(x)       __builtin_expect((x),1)*/
/*#define unlikely(x)     __builtin_expect((x),0)*/
#define likely(x)       x
#define unlikely(x)     x
#define printf(...)  

#define gen_id()        ++id_counter

extern void __lwt_trampoline(void);

lwt_queue_t run_queue;
lwt_t active_thread;
unsigned long id_counter;
unsigned long n_runnable, n_blocked, n_zombies;

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
        printf("schedule from thread %lu to thread %lu\n", curr->id, next->id);
        printf("curr->state is %d\n", curr->state);
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
                if (run_queue->tail == node) run_queue->tail = NULL;
        }
        return node;
}


inline lwt_t
lwt_create(lwt_fn_t fn, void *data)
{
        /* 1. allocate memory for thread */
        lwt_t thd = malloc(sizeof(struct lwt));

        /* 2. set up the stack */
        /* stack bottom: data, fn, _trampoline, 0, 0, 0, 0, sp, bp, 0, 0 */
        thd->id = gen_id();
        printf("thread %lu created thread %lu\n", lwt_current()->id, thd->id);
        thd->mem_space = malloc(STACK_SIZE);
        thd->sp = (unsigned long)thd->mem_space + STACK_SIZE;
        thd->ret = NULL;
        thd->state = RUNNABLE;
        n_runnable++;
        thd->parent = lwt_current();
        thd->next = NULL;

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
        lwt_t curr = lwt_current();
        if (unlikely(child->parent != curr)) {
                printf("can only join by parent !!!\n");
                return NULL;
        }
        curr->state = BLOCKED;
        n_blocked++;
        n_runnable--;
        while (child->state != ZOMBIE) {
                printf("waiting for thread %lu to finish ...\n", child->id);
                __lwt_schedule();
        }
        printf("%lu joined %lu\n", lwt_current()->id, child->id);
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
        printf("thread %lu died\n", curr->id);
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
                default:
                        ret = 0;
        }

        return ret;
}
