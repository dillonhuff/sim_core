#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mat2_3_add.h"

int main() {
  /* uint64_t** A = (uint64_t**) malloc(sizeof(uint64_t)*6); */
  /* uint64_t** B = (uint64_t**) malloc(sizeof(uint64_t)*6); */
  /* uint64_t** C = (uint64_t**) malloc(sizeof(uint64_t)*6); */

  uint64_t A[2][3];
  uint64_t B[2][3];
  uint64_t C[2][3];

  printf("About to simulate\n");

  simulate(A, B, &C);

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 3; j++) {
      printf("C[%d][%d] = %llu\n", i, j, C[i][j]);
    }
  }

}
