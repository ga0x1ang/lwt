#include <lwt.h>

struct clist_head {
        int start;
        int end;
        int size;
        lwt_t *data;
};

int clist_init(struct clist_head *list, int size);
lwt_t clist_get(struct clist_head *list);
void clist_add(struct clist_head *list, lwt_t thd);
int clist_destroy(struct clist_head *list);
