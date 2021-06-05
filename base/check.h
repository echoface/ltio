#ifndef _LT_BASE_CHECK_H_H
#define _LT_BASE_CHECK_H_H

#define CHECK(x)                                     \
  do {                                               \
    if (!(x)) {                                      \
      fprintf(stderr, "%s:%d: ", __func__, __LINE__);\
      perror(#x);                                    \
      exit(-1);                                      \
    }                                                \
  } while (0)

#define CHECK_NE(x, y) CHECK((x != y))                                     \

#endif
