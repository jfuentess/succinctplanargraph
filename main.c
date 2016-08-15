/******************************************************************************
 * main.c
 *
 * Parallel construction of succinct triangulated plane graphs
 * For more information: http://thesis.josefuentes.cl
 *
 ******************************************************************************
 * Copyright (C) 2016 José Fuentes Sepúlveda <jfuentess@udec.cl>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "parallel_succinct_graph.h"
#include "succinct_tree.h"

int main(int argc, char** argv) {

  struct timespec stime, etime;
  double time;

  if(argc < 3) {
    fprintf(stderr, "Usage: %s <input graph> <input spanning tree> <input\
    canonical ordering>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  Graph* g = read_graph_from_file(argv[1]);
  Tree* t = read_tree_from_file(argv[2]);

  // Adding the canonical ordering the graph g
  uint* co = read_canonical_ordering_from_file(argv[3], g->n);
  for(uint i = 0; i < g->n; i++) {
    g->V[i].order = co[i];
  }
  free(co);


#ifdef MALLOC_COUNT
  size_t s_total_memory = malloc_count_total();
  size_t s_current_memory = malloc_count_current();
  malloc_reset_peak();
#else
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID , &stime)) {
    fprintf(stderr, "clock_gettime failed");
    exit(-1);
  }
#endif
  
  succ_graph* sg = parallel_succinct_graph(g, t);

#ifdef MALLOC_COUNT
  size_t e_total_memory = malloc_count_total();
  size_t e_current_memory = malloc_count_current();
  printf("%s,%ld,%zu,%zu,%zu,%zu,%zu\n", argv[1], t->n, s_total_memory,
  e_total_memory, malloc_count_peak(), s_current_memory, e_current_memory);
  
#else
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID , &etime)) {
    fprintf(stderr, "clock_gettime failed");
    exit(-1);
  }
  
  time = (etime.tv_sec - stime.tv_sec) + (etime.tv_nsec - stime.tv_nsec) / 1000000000.0;
  printf("%d,%s,%u,%lf\n", threads, argv[1], g->n, time);
#endif

  return EXIT_SUCCESS;
}
