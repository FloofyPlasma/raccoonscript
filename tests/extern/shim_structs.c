#include <stdint.h>

struct Point {
  int32_t x;
  int32_t y;
};

int32_t rac_point_sum(struct Point *p) {
  if (p) {
    return p->x + p->y;
  }
  return 0;
}
