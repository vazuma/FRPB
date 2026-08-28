#pragma once

// ---------------------------------------------------------------------------
// Classic families of graphs.  Each generator fills a Graph and lays the nodes
// out in a sensible starting position, so that the force-directed simulation
// has something better than random noise to relax from.
// ---------------------------------------------------------------------------

#include "graph.h"

namespace GraphGenerators {

Graph empty(int n);                              // n isolated vertices
Graph path(int n);                               // P_n
Graph cycle(int n);                              // C_n
Graph complete(int n);                           // K_n
Graph completeBipartite(int m, int n);           // K_{m,n}
Graph grid(int cols, int rows);                  // the cols x rows lattice
Graph star(int leaves);                          // K_{1,n}
Graph wheel(int rim);                            // W_n
Graph tree(int depth, int branching);            // full b-ary tree
Graph petersen();                                // the Petersen graph
Graph hypercube(int dimension);                  // Q_d
Graph erdosRenyi(int n, double p, quint32 seed); // G(n, p)

} // namespace GraphGenerators
