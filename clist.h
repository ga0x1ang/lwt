#include <lwt.h>

struct clist_head {
        int start;
        int end;
        int size;
        void **data;
};

int clist_init(struct clist_head *list, int size);
void *clist_get(struct clist_head *list);
int clist_add(struct clist_head *list, void *data);
int clist_destroy(struct clist_head *list);
