#include <clist.h>

int
clist_init(struct clist_head *list, int size)
{
        list->size = size + 1;
        list->start = 0;
        list->end = 0;
        list->data = malloc(size * sizeof(void *));

        return 0;
}

void *
clist_get(struct clist_head *list)
{
        if (list->end == list->start) return NULL;
        void *ret = list->data[list->start];
        list->start = (list->start + 1) % list->size;

        return ret;
}

int
clist_add(struct clist_head *list, void *data)
{
        int new_end = (list->end + 1) % list->size;
        if (new_end == list->start) return -1; /* if ring buffer is full */
        list->data[list->end] = data;
        list->end = new_end;

        return 0;
}

int
clist_destroy(struct clist_head *list)
{
        /*free(list);*/
        return 0;
}
