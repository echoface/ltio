/*
 * timeout.h - Tickless hierarchical timing wheel.
 * Copyright (c) 2013, 2014  William Ahern
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <inttypes.h> /* PRIu64 PRIx64 PRIX64 uint64_t */
#include <stdbool.h>  /* bool */
#include <stdio.h>    /* FILE */

#include <sys/queue.h> /* TAILQ(3) */

/* integer type interfaces */

#define TIMEOUT_C(n) UINT64_C(n)

#define TIMEOUT_mHZ TIMEOUT_C(1000)
#define TIMEOUT_uHZ TIMEOUT_C(1000000)
#define TIMEOUT_nHZ TIMEOUT_C(1000000000)

typedef uint64_t timeout_t;

#define timeout_error_t int /* for documentation purposes */

#ifndef TIMEOUT_CB_OVERRIDE
/**
 * callback interface
 *
 * Callback function parameters unspecified to make embedding into existing
 * applications easier.
 */
struct timeout_cb {
    void (*fn)();
    void *arg;
};
#endif

/* timeout interfaces */

#ifndef TIMEOUT_DISABLE_INTERVALS
#define TIMEOUT_INT 0x01 /* interval (repeating) timeout */
#endif
#define TIMEOUT_ABS 0x02 /* treat timeout values as absolute */

#define TIMEOUT_INITIALIZER(flags) \
    {                              \
        (flags)                    \
    }

#define timeout_setcb(to, _fn, _arg)  \
    do {                            \
        (to)->callback.fn = (_fn);   \
        (to)->callback.arg = (_arg); \
    } while (0)

struct timeout {
    int flags;

    timeout_t expires; /**< absolute expiration time */

    /** timeout list if pending on wheel or expiry queue */
    struct timeout_list *pending;

    /** entry member for struct timeout_list lists */
    LIST_ENTRY(timeout) le;

#ifndef TIMEOUT_DISABLE_CALLBACKS
    struct timeout_cb callback; /**< ptional callback information */
#endif

#ifndef TIMEOUT_DISABLE_INTERVALS
    timeout_t interval; /**< timeout interval if periodic */
#endif

#ifndef TIMEOUT_DISABLE_RELATIVE_ACCESS
    struct timeouts *timeouts; /**< timeouts collection if member of */
#endif
};

/** initialize timeout structure (same as TIMEOUT_INITIALIZER) */
struct timeout *timeout_init(struct timeout *, int);

#ifndef TIMEOUT_DISABLE_RELATIVE_ACCESS
/** true if on timing wheel, false otherwise */
bool timeout_pending(struct timeout *);

/** true if on expired queue, false otherwise */
bool timeout_expired(struct timeout *);

/** remove timeout from any timing wheel (okay if not member of any) */
void timeout_del(struct timeout *);
#endif

/* timing wheel interfaces */

struct timeouts;

/** open a new timing wheel, setting optional HZ (for float conversions) */
struct timeouts *timeouts_open(timeout_t, timeout_error_t *);

/** destroy timing wheel */
void timeouts_close(struct timeouts *);

/** return HZ setting (for float conversions) */
timeout_t timeouts_hz(struct timeouts *);

void timeouts_update(struct timeouts *, timeout_t);
/* update timing wheel with current absolute time */

/** step timing wheel by relative time */
void timeouts_step(struct timeouts *, timeout_t);

/** return interval to next required update */
timeout_t timeouts_timeout(struct timeouts *);

/** add timeout to timing wheel */
void timeouts_add(struct timeouts *, struct timeout *, timeout_t);

/** remove timeout from any timing wheel or expired queue (okay if on neither)
 */
void timeouts_del(struct timeouts *, struct timeout *);

/** return any expired timeout (caller should loop until NULL-return) */
struct timeout *timeouts_get(struct timeouts *);

/** return true if any timeouts pending on timing wheel */
bool timeouts_pending(struct timeouts *);

bool timeouts_expired(struct timeouts *);
/* return true if any timeouts on expired queue */

/** return true if invariants hold. describes failures to optional file handle.
 */
bool timeouts_check(struct timeouts *, FILE *);

#define TIMEOUTS_PENDING 0x10
#define TIMEOUTS_EXPIRED 0x20
#define TIMEOUTS_ALL (TIMEOUTS_PENDING | TIMEOUTS_EXPIRED)
#define TIMEOUTS_CLEAR 0x40

#define TIMEOUTS_IT_INITIALIZER(flags) \
    {                                  \
        (flags), 0, 0, 0, 0            \
    }

#define TIMEOUTS_IT_INIT(cur, _flags) \
    do {                              \
        (cur)->flags = (_flags);      \
        (cur)->pc = 0;                \
    } while (0)

struct timeouts_it {
    int flags;
    unsigned pc, i, j;
    struct timeout *to;
};

/**
 * return next timeout in pending wheel or expired queue. caller can delete
 * the returned timeout, but should not otherwise manipulate the timing
 * wheel. in particular, caller SHOULD NOT delete any other timeout as that
 * could invalidate cursor state and trigger a use-after-free.
 */
struct timeout *timeouts_next(struct timeouts *, struct timeouts_it *);

#define TIMEOUTS_FOREACH(var, T, flags)                        \
    struct timeouts_it _it = TIMEOUTS_IT_INITIALIZER((flags)); \
    while (((var) = timeouts_next((T), &_it)))

#endif /* TIMEOUT_H */
