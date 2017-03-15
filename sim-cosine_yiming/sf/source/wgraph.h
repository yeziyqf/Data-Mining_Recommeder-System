/*
	Graph Management functions
	CM 2007
  DB 2011
*/
#ifndef WGRAPH_H
#define WGRAPH_H

#include <stdio.h>

const double DEFAULT_WEIGHT=1.0;

typedef struct _Link {
  int node;
  double weight;
} Link;

typedef struct _Graph {
  int n;
  long int m;
  Link **links;
  int *degrees;
  int *capacities;
} Graph;

void free_graph_old_start(Graph *g, int old_0);
void free_graph(Graph *g);
Graph *wgraph_from_path(const char* path);
Graph *wgraph_from_file(FILE *f);
Graph *graph_from_path(const char* path);
Graph *graph_from_file(FILE *f);
// void sort_graph(Graph *g);
// int *sort_nodes_by_degrees(Graph *g); /* in O(m) time and O(n) space */
// void renumbering(Graph *g, int *perm);
// int random_renumbering(Graph *g);
#endif
