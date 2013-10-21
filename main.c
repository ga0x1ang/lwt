#include "lwt.h"

void *test_func(void *data);
unsigned long long begin, end;

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
        lwt_t curr = lwt_current()->data;
        lwt_yield(NULL);

        //printf("thread %lu begin to run\n", id);
        /*
        if (id_counter <= 10 ) {
                lwt_t thd = lwt_create(test_func, "sub-child");
                lwt_yield(NULL);
                lwt_join(thd);
        }
        */
        lwt_die(data);

        return data;
}
