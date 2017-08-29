#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "counter.h"

int main() {
  uint8_t self_en = 1;
  uint16_t* self_out_ptr = malloc(sizeof(uint16_t));
  uint8_t self_clk = 0;
  uint16_t ri_old_value = 0;

  uint16_t ri_new_value = 0;

  for (int i = 0; i < 20; i++) {
    self_clk = i % 2;
    simulate(self_en, self_out_ptr, self_clk, ri_old_value, &ri_new_value);
    printf("output = %hu\n", *self_out_ptr);
    printf("new_register value = %hu\n", ri_new_value);

    ri_old_value = ri_new_value;
  }

  

  free(self_out_ptr);
}
