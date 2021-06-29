#ifndef _LT_BASE_CHECK_H_H
#define _LT_BASE_CHECK_H_H

#define LTCHECK(x)                                    \
  do {                                                \
    if (!(x)) {                                       \
      fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
      perror(#x);                                     \
      exit(-1);                                       \
    }                                                 \
  } while (0)

#define LTCHECK_NE(x, y) LTCHECK((x != y))

#endif
