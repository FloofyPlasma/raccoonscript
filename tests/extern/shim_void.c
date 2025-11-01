#include <stdint.h>

void rac_increment(int32_t *ptr) {
  if (ptr) {
    (*ptr)++;
  }
}
