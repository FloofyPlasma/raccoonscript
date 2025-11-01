#include <stdint.h>

void rac_set_value(int32_t *ptr, int32_t value) {
  if (ptr) {
    *ptr = value;
  }
}

int32_t rac_get_value(int32_t *ptr) {
  if (ptr) {
    return *ptr;
  }
  return 0;
}
