#include "lwt.h"

void *test_func(void *data);

int main(void)
{
        lwt_t test = lwt_create(test_func, "874");
        lwt_t test2 = lwt_create(test_func, "634");
        printf("%p\n", *(lwt_fn_t *)(test2->sp - 4));

        __lwt_dispatch(test2, test);

        return 0;
}

void *
test_func(void *data)
{
        assert(0);
        printf("test func called: %s\n", (char *)data);

        return NULL;
}
