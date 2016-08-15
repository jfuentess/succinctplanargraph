/******************************************************************************
 * defs.c
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
#include "defs.h"
#include <math.h>

Graph* createGraph(uint n, uint m) {

  Graph *g = malloc(sizeof(Graph));
  g->n = n;
  g->m = m;
  g->V = malloc(g->n*sizeof(Vertex));
  g->E = malloc(2*(g->m)*sizeof(Edge));

  return g;
}

void deleteGraph(Graph* g){
  free(g->V);
  free(g->E);
  free(g);
}

uint degree(Vertex v) {
  return v.last - v.first + 1;
}

Vertex vertex(Graph *g, uint i) {
  return g->V[i];
}

Edge edge(Graph* g, uint e) {
  return g->E[e];
}

uint edges(Graph* g) {
  return g->m;
}

uint vertices(Graph* g) {
  return g->n;
}

uint firstE( Vertex v) {
  return v.first;
}

uint nextE(Vertex v, uint i) {
  return min(v.last,i+1);
}

uint prevE(Vertex v, uint i) {
  return max(v.first,i-1);
}

uint lastE(Vertex v) {
  return v.last;
}

uint pSource(Edge e) {
  return e.src;
}

uint pTarget(Edge e) {
  return e.tgt;
}

Edge complement(Graph *g, Edge e) {
  return g->E[e.p_tgt];
}

Vertex source(Graph *g, Edge e) {
  return g->V[e.src];
}

Vertex target(Graph *g, Edge e) {
  return g->V[e.tgt];
}


/*
Compute in parallel the prefix sum of an array of uints
Input: An array A of uints, the size the array and the number of available threads.
Output: None. The prefix sums will be saved in the array A

Note: This algorithm assumes that size >= threads
*/
void parallel_prefix_sum(uint* A, uint size) {
  uint chk = ceil((float)size/threads);
  cilk_for(uint i = 0; i < threads; i++) {
    uint  ll = i*chk, ul = ll + chk;
    if(ul > size)
      ul = size;

    uint acc = 0;
    for(uint j = ll; j < ul; j++) {
      A[j] += acc;
      acc = A[j];
    }
  }
  
  for(uint i = 1; i < threads-1; i++)
    A[((i+1)*chk)-1] += A[i*chk-1];
  
  if(threads > 1)
    A[size-1] += A[(threads-1)*chk-1];
  
  cilk_for(uint i = 1; i < threads; i++) {
    uint ll = i*chk, ul = ll + chk - 1;
    if(ul >= size)
      ul = size - 1;

    uint acc = A[ll-1];
    for(uint j = ll; j < ul; j++) {
      A[j] += acc;
    }
  }
  
}

void parallel_list_ranking(ENode* A, uint size) {  
   struct sublist_node {
     int head;
     int next;
     int scratch;
     int value;
   };

   uint s = ceil(log2(size)*threads);
   uint chk = size/s;

   struct sublist_node* sublist = malloc(s*sizeof(struct sublist_node));

   int curr2 = A[0].next;
   int cnt = 1;
   while(curr2 > 0) {
      curr2 = A[curr2].next;
      cnt++;
      if(cnt > size)
  	break;
   }
 
   // Compute the splitters
   cilk_for(uint i = 0; i < s; i++) {
     uint x = i*chk;
     sublist[i].head = x;
     sublist[i].value = A[x].rank;
     sublist[i].next = -1;
     sublist[i].scratch = A[x].next;
     A[x].next = -(i)-1;
   }
   
   cilk_for(uint i = 0; i < s; i++) {
     int curr = sublist[i].scratch;
     uint tmp = 0, tmp2 = 0;
    
    while(curr > 0) {
      tmp2 = A[curr].rank;
      A[curr].rank = tmp;
      tmp += tmp2;
      
      int aux = A[curr].next;
      A[curr].next = -(i)-1;
      curr = aux;
   }
    sublist[i].next = -(curr)-1;

    // Special case
    if(curr != 0)
      sublist[-(curr)-1].value = tmp;
   }
   
  int curr = 0;
  int tmp = 0, tmp2 = 0;

  while(1) {
    tmp2 = sublist[curr].value;
    sublist[curr].value += tmp;
    tmp += tmp2;
    curr = sublist[curr].next;
    if(curr < 0)
      break;
  }
  
  cilk_for(uint i = 0; i < s; i++) {
    uint  ll = i*chk, ul = ll + chk;
    if(i == s-1)
      ul = size;
    if(i == 0)
      ll++;

    for(uint j = ll; j < ul; j++) {
      int idx = -(A[j].next)-1;
      A[j].rank += sublist[idx].value;
    }
  }

  A[0].rank=0;
}
