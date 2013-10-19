#include "lwt.h"

lwt_queue_t run_queue;
lwt_node_t active_thread;
unsigned long thd_counter;

inline unsigned long 
gen_id(void)
{
        thd_counter++;
        return thd_counter;
}

__attribute__((constructor))
static void
lwt_init(void)
{
        /* construct tcb */
        lwt_t master = malloc(sizeof(struct lwt));
        master->id = gen_id();

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
lwt_enqueue(lwt_node_t node)
{
        if (run_queue->head == NULL) run_queue->head = run_queue->tail = node;
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
        //if (run_queue->head != NULL) {
                node = run_queue->head;
                run_queue->head = node->next;
        //}
        return node;
}

extern void __lwt_trampoline(void);

lwt_t
lwt_create(lwt_fn_t fn, void *data)
{
        /* 1. allocate memory for thread */
        lwt_t thd = malloc(sizeof(struct lwt));

        /* 2. set up the stack */
        /* stack bottom: data, fn, _trampoline, 0, 0, 0, 0, sp, bp, 0, 0 */
        thd->id = gen_id();
        thd->sp = (unsigned long)malloc(STACK_SIZE) + STACK_SIZE;
        unsigned long current_sp, restored_sp;
        thd->ret = NULL;

        /**
         * save current esp and switch to the stack we are going to construct 
         */
        __asm__ __volatile__("movl %%esp, %0\n\t" /* save current esp */
                             "movl %2, %%esp\n\t" /* switch to new stack */
                             "pushl $__lwt_trampoline\n\t"
                             "movl %%esp, %1\n\t"
                             "pushl %3\n\t" /* eax and  ecx are used to store*/
                             "pushl %4\n\t" /* args passed to __lwt_start */
                             "pushl $0\n\t"
                             "pushl $0\n\t"
                             "pushl %1\n\t"
                             "pushl %2\n\t"
                             "pushl $0\n\t"
                             "pushl $0\n\t"
                             "movl %%esp, %2\n\t"
                             "movl %0, %%esp"
                             : "=m" (current_sp), "=m" (restored_sp)
                             : "m" (thd->sp), "m" (fn), "m" (data)
                             : "esp");

        /* 3. add to run queue */
        lwt_node_t node = malloc(sizeof(struct lwt_node));
        node->data = thd;
        node->next = NULL;
        lwt_enqueue(node);

        return thd;
}

lwt_node_t
lwt_current(void)
{
        return active_thread;
}

void
lwt_die(void *ret)
{
        lwt_node_t curr = lwt_current();
        curr->data->state = JOINED;  /* TODO: or moved to free list ? */
        curr->data->ret = ret;

        return;
}

void *
lwt_join(lwt_t child)
{
        void *ret = NULL;
        if (child->state == JOINED) ret = child->ret;
        else __lwt_schedule();

        return ret;
}

int
lwt_yield(lwt_t next)
{
        if (run_queue->head == NULL) return;
        __lwt_schedule();
        return 0;
}

void
__lwt_dispatch(lwt_t next, lwt_t curr)
{
        __asm__ __volatile__("movl $1f, %1\n\t"
                             "pushl $1f\n\t"
                             "pushal\n\t"
                             "movl %%esp, %0\n\t"
                             "movl %2, %%esp\n\t"
                             "popal\n\t"
                             "pushl %1\n\t"
                             "1:\t"
                             : "=m" (curr->sp), "=m" (curr->ip)
                             : "m" (next->sp)
                             : "esp");

        return;
}

void
__lwt_schedule(void)
{
        lwt_node_t curr = lwt_current();
        if (curr->data->state != JOINED) lwt_enqueue(curr);
        lwt_node_t next = lwt_dequeue();
        printf("schedule from thread %lu to thread %lu\n", curr->data->id, next->data->id);
        if (next->data->state == JOINED) return;
        active_thread = next;
        __lwt_dispatch(next->data, curr->data);
        return;
}
