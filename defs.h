/******************************************************************************
 * defs.h
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

#include "bit_array.h"
#include <math.h>

#ifdef NOPARALLEL
#define cilk_for for
#define cilk_spawn
#define cilk_sync
#define __cilkrts_get_nworkers() 1
#else
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>
#include <cilk/common.h>
#endif

#ifdef MALLOC_COUNT
#include "malloc_count.h"
#endif


#define threads  __cilkrts_get_nworkers()

#define min(a,b)	      \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })

#define max(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

typedef struct _vertex_t Vertex;
typedef struct _edge_t Edge;
typedef struct _graph_t Graph;
typedef struct _node_t Node;
typedef struct _tree_t Tree;
typedef struct _euler_node ENode;
typedef unsigned int uint;
typedef struct _subgraph_node Subgraph;

// Used to represent graphs
struct _vertex_t {
  uint first; // Position of the first incident edge of a vertex in E
  uint last; // Position of the last incident edge of a vertex in E
  uint order; // Its order in the canonical ordering
};

// Used to represent graphs and trees
struct _edge_t {
  uint src; // Index of the source vertex of the edge
  uint tgt; // Index of the target vertex of the edge
  // uint p_src; // Position of the edge in the adjacency list of src
  // Note that p_src is the same index of the edge in E
  uint p_tgt; // Position of the edge in the adjacency list of tgt
};

// Used to represent trees
struct _node_t {
  uint first; // Position of the first incident edge of a node in E
  uint last; // Position of the last incident edge of a node in E
  //  uint size; // Number of nodes in the subtree rooted at this node
};

struct _graph_t {
  Vertex* V; // Array of vertices of the graph
  Edge* E; // Array of edges of the graph. It is the concatenation of the adjacency lists of all vertices
  uint n; // Number of vertices in the graph
  uint m; // Number of non-repeated edges in the graph
};

struct _tree_t {
  Node* N; // Array of nodes of the tree
  Edge* E; // Array of edges of the tree. It is the concatenation of the adjacency lists of all nodes
  uint n; // Number of nodes in the tree
  // The number of edges is n-1
};

// struct for the Euler tour code
struct _euler_node {
  int next; // stores the index of the next value in the array. Since
	    // the parallel_list_ranking algorithm uses this fields to
	    // store some negative values, it must be int instead of uint.
  char value;
  uint rank;
};

// Create a new graph with n vertices and m edges
Graph* createGraph(uint, uint);

// Free the memory used by a graph
void deleteGraph(Graph*);

// Return the degree of a vertex
uint degree(Vertex);

// Given a position, it returns the vertex at that position
Vertex vertex(Graph*, uint);

// Given a position, it returns the edge at that position
Edge edge(Graph*, uint);

// Return the number of vertices in the graph
uint vertices(Graph* g);

// Return the number of edges in the graph
uint edges(Graph* g);

// Given a vertex, it returns the position of the first edge of that vertex
uint firstE(Vertex);

// Given a vertex and the position of a edge, it returns the position
// of the next edge of that vertex in ccw order
uint nextE(Vertex, uint i);

// Given a vertex and the position of a edge, it returns the position
// of the previous edge of that vertex in ccw order
uint prevE(Vertex, uint i);

// Given a vertex, it returns the position of the last edge of that vertex
uint lastE(Vertex);

// Given an edge, it returns the position of the source vertex of that edge
uint pSource(Edge);

// Given an edge, it returns the position of the target vertex of that edge
uint pTarget(Edge);

// Given an edge, it returns the complement edge of that edge
Edge complement(Graph*, Edge);

// Given an edge, it returns the source vertex of that edge
Vertex source(Graph*, Edge);

// Given an edge, it returns the target vertex of that edge
Vertex target(Graph*, Edge);

/*============= LIST RANKING ==================*/

void parallel_prefix_sum(uint*, uint);
void parallel_list_ranking(ENode*, uint);

