#include "lwt.h"

void *test_func(void *data);

int
main(void)
{
        lwt_t thd1 = lwt_create(test_func, "634");
        lwt_t thd2 = lwt_create(test_func, "abc");
        lwt_yield(NULL);
        printf("i'm back\n");
        lwt_yield(NULL);
        printf("recieved return value %d\n", (int)lwt_join(thd1));
        printf("recieved return value %d\n", (int)lwt_join(thd2));
        printf("hooray!\n");
        lwt_yield(NULL);
        lwt_yield(NULL);
        lwt_yield(NULL);
        lwt_yield(NULL);
        lwt_yield(NULL);
        lwt_yield(NULL);
        lwt_yield(NULL);


        return 0;
}

void *
test_func(void *data)
{
        lwt_node_t curr = lwt_current();
        printf("i'm thread %lu\n", curr->data->id);
        lwt_yield(NULL);
        printf("test func called: %s\n", (char *)data);
        void *ret = (void *)634;
        lwt_die(ret);

        return ret;
}
