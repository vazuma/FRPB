#pragma once

// ---------------------------------------------------------------------------
// NodeListModel - a QAbstractListModel over the controller's nodes.
//
// This is the idiomatic way to feed a list of things to QML: instead of
// rebuilding a JavaScript array every frame, you expose a model with named
// roles and let ListView / Repeater consume it.  Roles become properties on
// the delegate, e.g. `text: label` or `x: worldX`.
// ---------------------------------------------------------------------------

#include <QAbstractListModel>
#include <QQmlEngine>

class GraphController;

class NodeListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS          // reachable from QML as a property, not constructible in QML

public:
    enum Roles {
        LabelRole = Qt::UserRole + 1,
        WorldXRole,
        WorldYRole,
        DegreeRole,
        FillColourRole,
        PinnedRole,
        IndexRole,
    };
    Q_ENUM(Roles)

    explicit NodeListModel(GraphController *controller);

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Called by the controller.  Structural changes need begin/endResetModel;
    // pure movement only needs a dataChanged() over the position roles.
    void beginRebuild();
    void endRebuild();
    void notifyPositionsChanged();
    void notifyStylingChanged();

private:
    GraphController *m_controller;
};
