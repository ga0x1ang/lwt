#include "lwt.h"
#include "string.h" /* for the fucking memcpy */ 

void
lwt_enqueue(lwt_node_t node, lwt_queue_t q)
{
        if (unlikely(q == NULL)) q = calloc(1, sizeof(struct lwt_queue));
        if (q->tail == NULL) {
                q->head = q->tail = node;
                return;
        }
        q->tail->next = node;
        q->tail = node;
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

        /**
         * save current esp and switch to the stack we are going to construct 
         */
        __asm__ __volatile__("movl %%esp, %0\n\t" /* save current esp */
                             "movl %1, %%esp\n\t" /* switch to new stack */
                             "pushl $__lwt_trampoline\n\t"
                             "pushl $0\n\t"
                             "pushl $0\n\t"
                             "pushl $0\n\t"
                             "pushl $0\n\t"
                             "pushl %1\n\t"
                             "pushl %2\n\t"
                             "pushl $0\n\t"
                             "pushl $0\n\t"
                             "movl %%esp, %2\n\t"
                             "movl %0, %%esp"
                             : "=m" (current_sp) : "m" (restored_sp), "m" (thd->sp) : "esp");

        /* 3. add to run queue */
        /*
        lwt_node_t node = malloc(sizeof(struct lwt_node));
        node->data = thd;
        node->next = NULL;
        lwt_enqueue(node, run_queue);
        */

        return thd;
}

void
__lwt_start(int data)
{
        printf("test %d\n", data);
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

