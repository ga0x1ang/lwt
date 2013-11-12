#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <lwt.h>
#include <clist.h>

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))

#define ITER 10000

/* 
 *  * My performance on an Intel Core i5-2520M CPU @ 2.50GHz:
 *   * Overhead for fork/join is 128
 *    * Overhead of yield is 13
 *     * Overhead for snd+rcv is 43
 *      */

void *
fn_bounce(void *d) 
{
        int i;
        unsigned long long start, end;

        lwt_yield(LWT_NULL);
        lwt_yield(LWT_NULL);
        rdtscll(start);
        for (i = 0 ; i < ITER ; i++) lwt_yield(LWT_NULL);
        rdtscll(end);
        lwt_yield(LWT_NULL);
        lwt_yield(LWT_NULL);

        if (!d) printf("Overhead of yield is %lld\n", (end-start)/(ITER*2));

        return NULL;
}

void *
fn_null(void *d)
{ return NULL; }

#define IS_RESET()                                              \
        assert( lwt_info(LWT_INFO_NTHD_RUNNABLE) == 1 &&        \
                        lwt_info(LWT_INFO_NTHD_ZOMBIES) == 0 &&         \
                        lwt_info(LWT_INFO_NTHD_BLOCKED) == 0)

void
test_perf(void)
{
        lwt_t chld1, chld2;
        int i;
        unsigned long long start, end;


        /* Performance tests */
        rdtscll(start);
        for (i = 0 ; i < ITER ; i++) {
                chld1 = lwt_create(fn_null, NULL, 0);
                lwt_join(chld1);
        }
        rdtscll(end);
        printf("Overhead for fork/join is %lld\n", (end-start)/ITER);
        IS_RESET();

        chld1 = lwt_create(fn_bounce, (void*)1, 0);
        chld2 = lwt_create(fn_bounce, NULL, 0);
        lwt_join(chld1);
        lwt_join(chld2);
        IS_RESET();
}

void *
fn_identity(void *d)
{ return d; }

void *
fn_nested_joins(void *d)
{
        lwt_t chld;

        if (d) {
                lwt_yield(LWT_NULL);
                lwt_yield(LWT_NULL);
                assert(lwt_info(LWT_INFO_NTHD_RUNNABLE) == 1);
                lwt_die(NULL);
        }
        chld = lwt_create(fn_nested_joins, (void*)1, 0);
        lwt_join(chld);
}

volatile int sched[2] = {0, 0};
volatile int curr = 0;

void *
fn_sequence(void *d)
{
        int i, other, val = (int)d;

        for (i = 0 ; i < ITER ; i++) {
                other = curr;
                curr  = (curr + 1) % 2;
                sched[curr] = val;
                assert(sched[other] != val);
                lwt_yield(LWT_NULL);
        }

        return NULL;
}

void *
fn_join(void *d)
{
        lwt_t t = (lwt_t)d;
        void *r;

        r = lwt_join(d);
        assert(r != (void*)0x37337);
}

void
test_crt_join_sched(void)
{
        lwt_t chld1, chld2;

        /* functional tests: scheduling */
        lwt_yield(LWT_NULL);

        chld1 = lwt_create(fn_sequence, (void*)1, 0);
        chld2 = lwt_create(fn_sequence, (void*)2, 0);
        lwt_join(chld2);
        lwt_join(chld1);        
        IS_RESET();

        /* functional tests: join */
        chld1 = lwt_create(fn_null, NULL, 0);
        lwt_join(chld1);
        IS_RESET();

        chld1 = lwt_create(fn_null, NULL, 0);
        lwt_yield(LWT_NULL);
        lwt_join(chld1);
        IS_RESET();

        chld1 = lwt_create(fn_nested_joins, NULL, 0);
        lwt_join(chld1);
        IS_RESET();

        /* functional tests: join only from parents */
        chld1 = lwt_create(fn_identity, (void*)0x37337, 0);
        chld2 = lwt_create(fn_join, chld1, 0);
        lwt_yield(LWT_NULL);
        lwt_yield(LWT_NULL);
        lwt_join(chld2);
        lwt_join(chld1);
        IS_RESET();

        /* functional tests: passing data between threads */
        chld1 = lwt_create(fn_identity, (void*)0x37337, 0);
        assert((void*)0x37337 == lwt_join(chld1));
        IS_RESET();

        /* functional tests: directed yield */
        chld1 = lwt_create(fn_null, NULL, 0);
        lwt_yield(chld1);
        assert(lwt_info(LWT_INFO_NTHD_ZOMBIES) == 1);
        lwt_join(chld1);
        IS_RESET();
}

void *
fn_chan(void *data)
{
        lwt_chan_t from;
        lwt_chan_t to = data;
        int i;

        from = lwt_chan(0);
        lwt_snd_chan(to, from);
        for (i = 0 ; i < ITER ; i++) {
                lwt_snd(to, (void*)1);
                assert(2 == (int)lwt_rcv(from));
        }
        lwt_chan_deref(from);

        return NULL;
}

void
test_perf_channels(void)
{
        lwt_chan_t from, to;
        lwt_t t;
        int i;
        unsigned long long start, end;

        assert(RUNNABLE == lwt_current()->state);
        from = lwt_chan(0);
        assert(from);
        t    = lwt_create(fn_chan, from, 0);
        to   = lwt_rcv_chan(from);
        rdtscll(start);
        for (i = 0 ; i < ITER ; i++) {
                assert(1 == (int)lwt_rcv(from));
                lwt_snd(to, (void*)2);
        }
        lwt_chan_deref(to);
        rdtscll(end);
        printf("Overhead for snd/rcv is %lld\n", (end-start)/(ITER*2));
        lwt_join(t);
}

struct multisend_arg {
        lwt_chan_t c;
        int snd_val;
};

void *
fn_snder(void *arg)
{
        struct multisend_arg *a = arg;
        lwt_chan_t c = a->c;
        int v = a->snd_val;

        lwt_snd(c, (void*)v);
        lwt_snd(c, (void*)v);

        return NULL;
}

void
test_multisend(void)
{
        lwt_chan_t c;
        lwt_t t1, t2;
        int i, ret[4], sum = 0;
        struct multisend_arg args[2];

        c  = lwt_chan(0);
        assert(c);
        for (i = 0 ; i < 2 ; i++) {
                args[i].c       = c;
                args[i].snd_val = i+1;
        }
        t1 = lwt_create(fn_snder, &args[0], 0);
        t2 = lwt_create(fn_snder, &args[1], 0);
        for (i = 0 ; i < 4 ; i++) ret[i] = (int)lwt_rcv(c);
        lwt_join(t1);
        lwt_join(t2);

        for (i = 0 ; i < 4 ; i++) {
                sum += ret[i];
                assert(ret[i] == 1 || ret[i] == 2);
        }
        assert(sum == 6);

        return;
}

int
main(void)
{
        test_perf();
        test_crt_join_sched();
        test_perf_channels();
        test_multisend();
}
