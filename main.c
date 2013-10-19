#include "lwt.h"

void *test_func(void *data);


int main(void)
{
        lwt_t test = lwt_create(test_func, "634");

        lwt_t curr = lwt_current();
        lwt_yield(NULL);

        return 0;
}

void *
test_func(void *data)
{
        lwt_t curr = lwt_current();
        printf("i'm thread %lu\n", curr->id);
        printf("test func called: %s\n", (char *)data);

        return (void *)634;
}
