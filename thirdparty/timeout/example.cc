#include <iostream>
#include "timeout.h"
#include <limits.h>
#include <vector>

static timeout_t random_to(timeout_t min, timeout_t max) {
	if (max <= min)
		return min;
	/* Not actually all that random, but should exercise the code. */
	timeout_t rand64 = random() * (timeout_t)INT_MAX + random();
	return min + (rand64 % (max-min));
}
int main() {

    int err = 0;
    srand (time(NULL));
    struct timeouts *tos = timeouts_open(TIMEOUT_uHZ, &err);
    timeouts_update(tos, 1000);
    timeout_t random_timeout_ms[10];
    struct timeout timers[10] = {0};

    for (int i = 0; i < 10; i++) {
        timeout_init(&timers[i], TIMEOUT_ABS);
		    random_timeout_ms[i] = random_to(0, 20000);
        std::cout << "timeout:" << random_timeout_ms[i]  << std::endl;
		    timeouts_add(tos, &timers[i], random_timeout_ms[i]);
    }

	  struct timeout *to = NULL;
    while (NULL != (to = timeouts_get(tos))) {
      std::cout << "expire time:" << to->expires << std::endl;
    }
    std::cout << "origin next tick:" << timeouts_timeout(tos) << std::endl;

    timeouts_update(tos, 1001);
    std::cout << "+1 next tick:" << timeouts_timeout(tos) << std::endl;

    timeouts_update(tos, 800);
    while (NULL != (to = timeouts_get(tos))) {
      std::cout << "roolback expire time:" << to->expires << std::endl;
    }
    std::cout << "rollback next tick:" << timeouts_timeout(tos) << std::endl;

    {
      timeout* to = NULL;
      TIMEOUTS_FOREACH(to, tos, TIMEOUTS_ALL) {
        std::cout << "foreach expiretime:" << to->expires << std::endl;
        timeouts_del(tos, to);
      }
      timeouts_close(tos);
    }
    return 0;
}
