#include <clist.h>

int
clist_init(struct clist_head *list, int size)
{
        list->size = size + 1;
        list->start = 0;
        list->end = 0;
        list->data = (lwt_t *)malloc(size * sizeof(lwt_t));

        return 0;
}

lwt_t
clist_get(struct clist_head *list)
{
        lwt_t ret = NULL;
        ret = list->data[list->start];
        list->start = (list->start + 1) % list->size;

        return ret;
}

void
clist_add(struct clist_head *list, lwt_t data)
{
        list->data[list->end] = data;
        list->end = (list->end + 1) % list->size;

        return;
}

int
clist_destroy(struct clist_head *list)
{
        return 0;
}
