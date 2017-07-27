#pragma once

#include <QTableView>


class QContextMenuEvent;
class QAction;

class ChannelConfigView : public QTableView
{
    Q_OBJECT

public:
    ChannelConfigView(QWidget * parent = 0);
    void Select(Qt::CheckState state);
    void SetContextMenu(QMenu* menu);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;


    QMenu* contextMenu;
};
