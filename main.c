#include "lwt.h"

void *test_func(void *data);


int main(void)
{
        lwt_t test = lwt_create(test_func, "634");
        __lwt_dispatch(test, curr);
        assert(0);

        return 0;
}

void *
test_func(void *data)
{
        printf("test func called: %s\n", (char *)data);

        return (void *)634;
}
