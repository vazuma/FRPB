#pragma once

// ---------------------------------------------------------------------------
// A Fruchterman-Reingold force-directed layout, plus a few deterministic
// alternatives.  The simulation is stepped one frame at a time by
// GraphController's QTimer, so the UI stays responsive and you can watch the
// graph untangle itself.
// ---------------------------------------------------------------------------

#include "graph.h"

class ForceLayout
{
public:
    // Runs one iteration.  Returns the total displacement, which the caller can
    // use to decide the layout has converged.
    double step(Graph &g);

    void reheat(double temperature = -1.0);   // < 0 restores the default
    double temperature() const { return m_temperature; }
    bool   isCool() const { return m_temperature < 0.35; }

    // Tunables, exposed so the UI can play with them.
    double idealEdgeLength = 90.0;    // "k" in the paper
    double gravity         = 0.012;   // pulls components towards the origin
    double cooling         = 0.985;   // temperature multiplier per step

private:
    double m_temperature = 60.0;
};

namespace Layouts {
void circular(Graph &g, double radius = 0.0);
void grid(Graph &g, double spacing = 90.0);
void random(Graph &g, double extent = 320.0, quint32 seed = 0);
// Places nodes on concentric rings by BFS distance from `root`.
void radialTree(Graph &g, int root, double ringSpacing = 95.0);
}
