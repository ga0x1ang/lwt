#include "lwt.h"

void *test_func(void *data);

int
main(void)
{
        lwt_t test = lwt_create(test_func, "634");
        lwt_yield(NULL);
        printf("i'm back\n");
        lwt_yield(NULL);
        printf("recieved return value %d\n", (int)lwt_join(test));

        return 0;
}

void *
test_func(void *data)
{
        lwt_node_t curr = lwt_current();
        lwt_yield(NULL);
        printf("i'm thread %lu\n", curr->data->id);
        printf("test func called: %s\n", (char *)data);

        void *ret = (void *)634;
        lwt_die(ret);

        return ret;
}
