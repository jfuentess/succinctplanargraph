/******************************************************************************
 * parallel_succinct_graph.c
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

succ_graph* init_succ_graph(Graph* g, Tree* t) {
  succ_graph* sg = (succ_graph*)malloc(sizeof(succ_graph));
  sg->n = g->n;
  sg->m = g->m;
  
  return sg;
}

void free_succ_graph(succ_graph* sg) {
  free_rmMt(sg->S1);
  free_rmMt(sg->S2);
  free_rmMt(sg->S3);
  free(sg);
}

void print_succ_graph(succ_graph* sg) {
  fprintf(stderr, "Number of nodes: %lu\n", sg->n);
  fprintf(stderr, "Number of edges: %lu\n", sg->m);
  fprintf(stderr, "Length of S1: %lu\n", sg->S1->n);
  fprintf(stderr, "Length of S2: %lu\n", sg->S2->n);
  fprintf(stderr, "Length of S3: %lu\n", sg->S3->n);
}

succ_graph* parallel_succinct_graph(Graph* g, Tree* t) {
  /* Extra operations to increase artificially the workload of the
  algorithm. This is only for testing. */
  uint extraOps = 0;

  // Section 0
  succ_graph* sg = init_succ_graph(g, t);

  uint num_parentheses = 2*t->n;
  uint num_brackets = 2*(g->m - t->n + 1);
  uint num_total = num_parentheses + num_brackets;

  BIT_ARRAY* S1 = bit_array_create(num_total);
  BIT_ARRAY* S2 = bit_array_create(num_parentheses);
  BIT_ARRAY* S3 = bit_array_create(num_brackets);

  ushort* lower_numb = calloc(t->n,sizeof(ushort));
  ushort* higher_numb = calloc(t->n,sizeof(ushort));

  ENode* ET = malloc((num_parentheses-2)*sizeof(ENode)); // S1
  ENode* ET2 = malloc((num_parentheses-2)*sizeof(ENode)); // S2

  uint chk = 2*g->m/threads;

  cilk_for(uint h = 0; h < threads; h++) {
    uint ll = h*chk;
    uint ul = ll+chk;
    if(h == 0)
      ll = 1;
    if(h == threads-1)
      ul = 2*g->m;
  
    for(uint i = ll; i < ul; i++) {
      Edge e = g->E[i];
	
      if(g->V[e.src].order > g->V[e.tgt].order)
  	__sync_add_and_fetch(&lower_numb[e.src], (ushort)1);
      else
  	__sync_add_and_fetch(&higher_numb[e.src], (ushort)1);

      // NOTE: Used to increase the workload
      /* for(uint ii = 0; ii < extraOps; ii++) { */
      /*   __atomic_compare_exchange_n(&ET[i].next, &ET[i].next, NULL, 0, */
      /* 			      __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST ); */
      /* } */
    }
  }

  cilk_for(uint h = 0; h < threads; h++) {
    uint ll = h*chk;
    uint ul = ll+chk;
    if(h == 0)
      ll = 1;
    if(h == threads-1)
      ul = t->n;
    
    for(uint i = ll; i < ul; i++) {
      lower_numb[i]--;
      higher_numb[i] -= t->N[i].last - t->N[i].first;
    }
  }

  uint root_last = t->N[0].last;

  chk = num_parentheses/threads;
  cilk_for(uint h = 0; h < threads; h++) {
    uint ll = h*chk;
    uint ul = ll+chk;
    if(h == threads-1)
      ul = num_parentheses-2;

    for(uint i = ll; i < ul; i++) {

      /* // NOTE: Used to increase the workload */
      /* for(uint ii = 0; ii < extraOps; ii++) { */
      /* 	__atomic_compare_exchange_n(&ET[i].next, &ET[i].next, NULL, 0, */
      /* 				    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST ); */
      /* } */

      Edge e = t->E[i];
      Node src = t->N[e.src];
      Node tgt = t->N[e.tgt];

      // Forward edge
      if(g->V[e.src].order < g->V[e.tgt].order) {
  	ET2[i].value = 1;
  	ET[i].rank = lower_numb[t->E[i].tgt]+1;
  	ET2[i].rank = 1;

  	// Leaf
        if(tgt.first == tgt.last) {
  	  ET[i].next = e.p_tgt;
  	  ET2[i].next = e.p_tgt;
  	}
        else { // Connect to the first child of the tgt node
  	  ET[i].next = tgt.first+1;
  	  ET2[i].next = tgt.first+1;
  	}
      }
      else { // Backward edge
  	ET2[i].value = 0;
  	ET[i].rank = higher_numb[t->E[i].src]+1;
  	ET2[i].rank = 1;
  	// Root
  	if((e.tgt == 0) && (e.p_tgt == root_last)) {
  	  ET[i].next = 0;
  	  ET2[i].next = 0;
  	  continue;
  	}

  	if(e.p_tgt == t->N[e.tgt].last) {
  	  ET[i].next = t->N[e.tgt].first; // Parent of the tgt node
  	  ET2[i].next = t->N[e.tgt].first; // Parent of the tgt node
  	}
  	else {
  	  ET[i].next = e.p_tgt+1; // Parent of the tgt node
  	  ET2[i].next = e.p_tgt+1; // Parent of the tgt node
  	}
      }
    }
  }

  parallel_list_ranking(ET, num_parentheses-2);
  parallel_list_ranking(ET2, num_parentheses-2);

  chk = num_parentheses/threads;
  cilk_for(uint h = 0; h < threads; h++) {
    uint ll = h*chk;
    uint ul = ll+chk;
    if(h == threads-1)
      ul = num_parentheses-2;
    
    for(uint i = ll; i < ul; i++) {

      /* // NOTE: Used to increase the workload */
      /* for(uint ii = 0; ii < extraOps; ii++) { */
      /* 	__atomic_compare_exchange_n(&ET[i].next, &ET[i].next, NULL, 0, */
      /* 				    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST ); */
      /* } */

      parallel_or_bit_array_set_bit(S1,ET[i].rank+1);
      if(ET2[i].value)
	parallel_or_bit_array_set_bit(S2,ET2[i].rank+1);
      else {
	uint ll = ET[i].rank-ET2[i].rank;
	uint ul = ll + higher_numb[t->E[i].src];
	for(uint j=ll; j < ul; j++)
	  parallel_or_bit_array_set_bit(S3,j);
      }
    }
  }
  bit_array_set_bit(S1,0);
  bit_array_set_bit(S1,num_total-1);
  bit_array_set_bit(S2,0);

  free(lower_numb);
  free(higher_numb);
  free(ET);
  free(ET2);

  sg->S1 = st_create_emM(S1, bit_array_length(S1));
  sg->S2 = st_create_emM(S2, bit_array_length(S2));
  sg->S3 = st_create_emM(S3, bit_array_length(S3));

  return sg;
}

