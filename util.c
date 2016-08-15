/******************************************************************************
 * util.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

// Assuming that the indices of the vertices are contiguous
Graph* read_graph_from_file(const char* fn) {
  Graph *g = malloc(sizeof(Graph));
  /*
    First, we fill the number of vertices, the number of edges, the
    limits of the adjacency lists of each vertex, the source/target
    vertices of each edge and the position in the adjacency list in
    the source vertex of each edge.
   */

  FILE *fp = fopen(fn, "r");;
  char line_buffer[BUFSIZ]; /* BUFSIZ is defined if you include stdio.h */

  if (!fp) {
    fprintf(stderr, "Error opening file \"%s\".\n", fn);
    exit(EXIT_FAILURE);
  }

  g->n = atoi(fgets(line_buffer, sizeof(line_buffer), fp));
  g->m = atoi(fgets(line_buffer, sizeof(line_buffer), fp));

  g->V = calloc(g->n,sizeof(Vertex));
  g->E = calloc(2*(g->m),sizeof(Edge));

  uint source = 0, m = 0;
  char *target;

  while (fgets(line_buffer, sizeof(line_buffer), fp)) {
    source = atoi(strtok(line_buffer, " "));
    g->V[source].first = m;
    target = strtok(NULL, " ");

    while (target != NULL && strcmp(target,"\n")!=0){
      g->E[m].src = source;
      g->E[m].tgt = atoi(target);
      //      g->E[m].p_src = m; // This can be omitted
      target = strtok(NULL, " ");
      m++;
    }

    g->V[source].last = m-1;
  }

  /*
    Second, we fill the position of each edge in the adjacency list of
    the source and target vertices.
   */

  uint i = 0, j = 0;

  for(i = 0; i < 2*g->m; i++) {
    Vertex targetV = g->V[g->E[i].tgt];

    for(j = targetV.first; j <= targetV.last; j++) {
      if(g->E[j].tgt == g->E[i].src) {
	g->E[i].p_tgt = j;
	break;
      }
    }
  }

  return g;
}

uint* read_canonical_ordering_from_file(const char* fn, uint n) {
  uint* co = calloc(n, sizeof(uint));

  FILE *fp = fopen(fn, "r");;
  char line_buffer[BUFSIZ]; /* BUFSIZ is defined if you include stdio.h */

  if (!fp) {
    fprintf(stderr, "Error opening file \"%s\".\n", fn);
    exit(EXIT_FAILURE);
  }

 fgets(line_buffer, sizeof(line_buffer), fp);

  uint vertex = 0, order = 0;
  while (fgets(line_buffer, sizeof(line_buffer), fp)) {
    vertex = atoi(strtok(line_buffer, " "));
    order = atoi(strtok(NULL, " "));
    co[vertex] = order;
  }

  return co;
}


void* write_graph_to_file(const char* fn, Graph* g) {

  FILE* fp = fopen(fn, "w");

  if (!fp) {
    fprintf(stderr, "Error opening file \"%s\".\n", fn);
    exit(EXIT_FAILURE);
  }

  uint i = 0, j = 0;

  fprintf(fp, "%u\n", g->n);
  fprintf(fp, "%u\n", g->m);

  for(i = 0; i < g->n; i++) {
    fprintf(fp, "%u ", i);
    Vertex v = vertex(g,i);
    for(j = firstE(v); j <= lastE(v)-1; j++) {
      fprintf(fp, "%u ", pTarget(edge(g,j)));
    }
    fprintf(fp, "%u\n", pTarget(edge(g,lastE(v))));
  }
}

void* write_tree_to_file(const char* fn, Tree* g) {

  FILE* fp = fopen(fn, "w");;

  if (!fp) {
    fprintf(stderr, "Error opening file \"%s\".\n", fn);
    exit(EXIT_FAILURE);
  }

  uint i = 0, j = 0;

  fprintf(fp, "%u\n", g->n);

  for(i = 0; i < g->n; i++) {
    fprintf(fp, "%u ", i);
    Node v = g->N[i];
    for(j = v.first; j < v.last; j++) {
      fprintf(fp, "%u ", g->E[j].tgt);
    }

    fprintf(fp, "%u\n", g->E[v.last].tgt);
  }
}

// Assuming that the indices of the vertices are contiguous
/*
  Format of the expected input file:
  
  <number of nodes>
  <number of edges>
  <node> <adjacency list>
  <node> <adjacency list>
  ....

  where
  <adjacency list> : <node 1> <node 2> <node 3> ...
 */

// Buffer + char
Tree* read_tree_from_file(const char* fn) {
  Tree *t = malloc(sizeof(Tree));
  /*
    First, we fill the number of vertices, the number of edges, the
    limits of the adjacency lists of each vertex, the source/target
    vertices of each edge and the position in the adjacency list in
    the source vertex of each edge.
   */

  FILE *fp = fopen(fn, "r");;
  char line_buffer[BUFSIZ]; /* BUFSIZ is defined if you include stdio.h */

  if (!fp) {
    fprintf(stderr, "Error opening file \"%s\".\n", fn);
    exit(EXIT_FAILURE);
  }

  t->n = atoi(fgets(line_buffer, sizeof(line_buffer), fp));

  t->N = calloc(t->n,sizeof(Node));
  t->E = calloc(2*(t->n-1),sizeof(Edge));

  uint source = 0, m = 0;
  char *target;

  while (fgets(line_buffer, sizeof(line_buffer), fp)) {
    source = atoi(strtok(line_buffer, " "));
    t->N[source].first = m;
    target = strtok(NULL, " ");

    while (target != NULL && strcmp(target,"\n")!=0){
      t->E[m].src = source;
      t->E[m].tgt = atoi(target);
      //      g->E[m].p_src = m; // This can be omitted
      target = strtok(NULL, " ");
      m++;
    }

    t->N[source].last = m-1;
  }

  /*
    Second, we fill the position of each edge in the adjacency list of
    the source and target vertices.
   */

  uint i = 0, j = 0;

  for(i = 0; i < 2*(t->n-1); i++) {
    Node targetV = t->N[t->E[i].tgt];

    for(j = targetV.first; j <= targetV.last; j++) {
      if(t->E[j].tgt == t->E[i].src) {
	t->E[i].p_tgt = j;
	break;
      }
    }
  }

  return t;
}
void free_graph(Graph* g) {
  free(g->V);
  free(g->E);
  free(g);
}

void free_tree(Tree* t) {
  free(t->N);
  free(t->E);
  free(t);
}
