#include "lwt.h"

#define gen_id()        ++id_counter
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define printf(...)  

lwt_queue_t run_queue;
lwt_node_t active_thread;
unsigned long id_counter;

unsigned long n_runnable = 0, n_blocked = 0, n_zombies = 0;

__attribute__((constructor))
static void
lwt_init(void)
{
        id_counter = 0;
        /* construct tcb */
        lwt_t master = malloc(sizeof(struct lwt));
        master->id = gen_id();
        master->state = RUNNABLE;
        n_runnable++;

        /* add current (must be master thread) to run queue */
        /* TODO: enqueue should be in dispatch */
        lwt_node_t master_node = malloc(sizeof(struct lwt_node));
        master_node->data = master;
        master_node->next = NULL;

        /* init the run queue */
        run_queue = malloc(sizeof(struct lwt_queue));
        run_queue->head = run_queue->tail = NULL;
        active_thread = master_node;

        return;
}

void
__lwt_dispatch(lwt_t next, lwt_t curr)
{
        __asm__ __volatile__("pushl $1f\n\t"
                             "pushal\n\t"
                             "movl %%esp, %0\n\t"
                             "movl %2, %%esp\n\t"
                             "popal\n\t"
                             "ret\n\t"
                             "1:\t"
                             : "=m" (curr->sp), "=m" (curr->ip)
                             : "m" (next->sp)
                             : "esp");
                             

        return;
}

inline void
__lwt_schedule(void)
{
        if (run_queue->head == NULL) return;
        lwt_node_t next = lwt_dequeue();
        lwt_node_t curr = lwt_current();
        curr->next = NULL;
        if (curr->data->state != ZOMBIE) lwt_enqueue(curr);
        printf("schedule from thread %lu to thread %lu\n", curr->data->id, next->data->id);
        printf("curr->state is %d\n", curr->data->state);
        active_thread = next;
        __lwt_dispatch(next->data, curr->data);

        return;
}

void
lwt_enqueue(lwt_node_t node)
{
        if (unlikely(run_queue->tail == NULL)) run_queue->head = run_queue->tail = node;
        else {
                run_queue->tail->next = node;
                run_queue->tail = node;
        }
        return;
}

lwt_node_t
lwt_dequeue()
{
        lwt_node_t node = NULL;
        if (likely(run_queue->head != NULL)) {
                //printf("thread %lu dequeued, run_queue->head = %lu\n", node->data->id, run_queue->head->data->id);
                node = run_queue->head;
                run_queue->head = node->next;
                if (run_queue->tail == node) run_queue->tail = NULL;
        }
        return node;
}

extern void __lwt_trampoline(void);

lwt_t
lwt_create(lwt_fn_t fn, void *data)
{
        /* 1. allocate memory for thread */
        lwt_t thd = calloc(1, sizeof(struct lwt));

        /* 2. set up the stack */
        /* stack bottom: data, fn, _trampoline, 0, 0, 0, 0, sp, bp, 0, 0 */
        thd->id = gen_id();
        printf("thread %lu created thread %lu\n", lwt_current()->data->id, thd->id);
        thd->mem_space = calloc(1, STACK_SIZE);
        thd->sp = (unsigned long)thd->mem_space + STACK_SIZE;
        thd->ret = NULL;
        thd->state = RUNNABLE;
        n_runnable++;
        thd->parent = lwt_current()->data;

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
        lwt_node_t node = calloc(1, sizeof(struct lwt_node));
        node->data = thd;
        node->next = NULL;
        lwt_enqueue(node);

        return thd;
}

int
lwt_yield(lwt_t next)
{
        if (unlikely(next != LWT_NULL)) {
                while (run_queue->head->data != next) {
                        lwt_node_t requeue = lwt_dequeue();
                        lwt_enqueue(requeue);
                }
                __lwt_schedule();
        } else if (likely(run_queue->head != NULL)) __lwt_schedule();

        return 0;
}

void *
lwt_join(lwt_t child)
{
        lwt_node_t curr = lwt_current();
        if (child->parent != curr->data) {
                printf("can only join by parent !!!\n");
                return NULL;
        }
        curr->data->state = BLOCKED;
        n_blocked++;
        n_runnable--;
        while (child->state != ZOMBIE) {
                printf("waiting for thread %lu to finish ...\n", child->id);
                __lwt_schedule();
        }
        printf("%lu joined %lu\n", lwt_current()->data->id, child->id);
        void *ret = child->ret;
        free(child->mem_space);
        free(child);

        curr->data->state = RUNNABLE;
        n_blocked--;
        n_runnable++;
        n_zombies--;

        return ret;
}

void
lwt_die(void *ret)
{
        lwt_node_t curr = lwt_current();
        printf("thread %lu died\n", curr->data->id);
        curr->data->state = ZOMBIE;
        n_runnable--;
        n_zombies++;
        curr->data->ret = ret;

        __lwt_schedule();

        return;
}
inline unsigned long
lwt_id(lwt_t thd)
{
        return thd->id;
}

inline lwt_node_t
lwt_current(void)
{
        return active_thread;
}

inline unsigned long lwt_info(lwt_info_t type)
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
