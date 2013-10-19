#include "lwt.h"

void *test_func(void *data);

int
main(void)
{
        lwt_t thd = lwt_create(test_func, "child");
        lwt_yield(NULL);
        lwt_join(thd);

        return 0;
}

void *
test_func(void *data)
{
        printf("thread %lu begin to run\n", active_thread->data->id);
        if (thd_counter <=10 ) {
                lwt_t thd = lwt_create(test_func, "sub-child");
                lwt_yield(NULL);
                lwt_join(thd);
        }
        lwt_die(data);

        return data;
}
