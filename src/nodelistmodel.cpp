#include "nodelistmodel.h"
#include "graphcontroller.h"

NodeListModel::NodeListModel(GraphController *controller)
    : QAbstractListModel(controller)
    , m_controller(controller)
{
}

int NodeListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())          // a flat list has no children
        return 0;
    return m_controller->graph().nodeCount();
}

QVariant NodeListModel::data(const QModelIndex &index, int role) const
{
    const Graph &g = m_controller->graph();
    if (!index.isValid() || !g.isValidIndex(index.row()))
        return {};

    const Node &n = g.node(index.row());
    switch (role) {
    case LabelRole:      return n.label;
    case WorldXRole:     return n.pos.x();
    case WorldYRole:     return n.pos.y();
    case DegreeRole:     return g.degree(index.row());
    case PinnedRole:     return n.pinned;
    case IndexRole:      return index.row();
    case FillColourRole: {
        const QList<QColor> &fills = m_controller->nodeFillColours();
        return index.row() < fills.size() ? fills.at(index.row()) : QColor(Qt::white);
    }
    default:             return {};
    }
}

QHash<int, QByteArray> NodeListModel::roleNames() const
{
    // The byte-array names on the right are what QML delegates actually see.
    return {
        { LabelRole,      "label" },
        { WorldXRole,     "worldX" },
        { WorldYRole,     "worldY" },
        { DegreeRole,     "degree" },
        { FillColourRole, "fillColour" },
        { PinnedRole,     "pinned" },
        { IndexRole,      "nodeIndex" },
    };
}

void NodeListModel::beginRebuild()
{
    beginResetModel();
}

void NodeListModel::endRebuild()
{
    endResetModel();
}

void NodeListModel::notifyPositionsChanged()
{
    if (rowCount() == 0)
        return;
    // One dataChanged() spanning every row is far cheaper than a reset, because
    // the views keep their delegates and only re-evaluate the listed roles.
    emit dataChanged(index(0), index(rowCount() - 1), { WorldXRole, WorldYRole });
}

void NodeListModel::notifyStylingChanged()
{
    if (rowCount() == 0)
        return;
    emit dataChanged(index(0), index(rowCount() - 1), { FillColourRole, DegreeRole });
}
