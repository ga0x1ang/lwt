#include "lwt.h"

lwt_queue_t run_queue;
lwt_t curr;
unsigned long thd_counter;

static inline unsigned long 
gen_id(void) { return ++thd_counter; }

static inline lwt_t
get_curr(void) { return curr; }

__attribute__((constructor))
static void
lwt_init(void)
{
        printf("lwt lib loaded!\n");
        /* init the run queue */
        run_queue = malloc(sizeof(struct lwt_queue));
        
        /* construct tcb */
        lwt_t master = malloc(sizeof(struct lwt));
        master->id = gen_id();

        /* add current (must be master thread) to run queue */
        /* TODO: enqueue should be in dispatch */
        lwt_node_t master_node = malloc(sizeof(struct lwt_node));
        master_node->data = master;
        master_node->next = NULL;
        curr = master;

        return;
}

void
lwt_enqueue(lwt_node_t node)
{
        if (run_queue->tail == NULL) {
                run_queue->head = run_queue->tail = node;
                return;
        }
        run_queue->tail->next = node;
        run_queue->tail = node;
        return;
}

lwt_node_t
lwt_dequeue(lwt_queue_t q)
{
        lwt_node_t node = NULL;
        if (q->head != NULL) {
                node = q->head;
                q->head = q->head->next;
        }
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
        thd->sp = (unsigned long)malloc(STACK_SIZE) + STACK_SIZE;
        unsigned long current_sp, restored_sp = thd->sp - 4;

        printf("fn: %p, data: %p\n", fn, data);

        /**
         * save current esp and switch to the stack we are going to construct 
         */
        __asm__ __volatile__("movl %%esp, %0\n\t" /* save current esp */
                             "movl %1, %%esp\n\t" /* switch to new stack */
                             "pushl $0x888\n\t"
                             "pushl $__lwt_trampoline\n\t"
                             "pushl %3\n\t" /* eax and  ebx are used to store*/
                             "pushl %4\n\t" /* args passed to __lwt_start */
                             "pushl $0\n\t"
                             "pushl $0\n\t"
                             "pushl %1\n\t"
                             "pushl %2\n\t"
                             "pushl $0\n\t"
                             "pushl $0\n\t"
                             "movl %%esp, %2\n\t"
                             "movl %0, %%esp"
                             : "=m" (current_sp)
                             : "m" (restored_sp), "m" (thd->sp), "m" (fn), "m" (data)
                             : "esp");

        /* 3. add to run queue */
        //lwt_node_t node = malloc(sizeof(struct lwt_node));
        //node->data = thd;
        //node->next = NULL;
        //lwt_enqueue(node);

        return thd;
}

inline lwt_t
lwt_current(void)
{
        return curr;
}

void
lwt_die(void *ret)
{
        lwt_t curr = lwt_current();
        curr->state = DEAD;

}

void
__lwt_start(lwt_fn_t fn, void *data)
{
        lwt_die(fn(data));  /* TODO: is this sytle good or even correct? */ 

        return;
}


void
__lwt_dispatch(lwt_t next, lwt_t curr)
{
        __asm__ __volatile__("pushal\n\t"
                             "movl %%esp, %0\n\t"
                             "movl %1, %%esp\n\t"
                             "popal\n\t"
                             "ret\n\t"
                             : "=m" (curr->sp) : "m" (next->sp) : "esp");

        return;
}

