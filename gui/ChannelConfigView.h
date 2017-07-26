#pragma once

#include <QTableView>


class QContextMenuEvent;
class QAction;

class ChannelConfigView : public QTableView
{
    Q_OBJECT

public:
    ChannelConfigView(QWidget * parent = 0);

public slots:
    void SelectMuted();
    void SelectUnmuted();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void createActions();
    void Select(Qt::CheckState state);

    QAction* actSelMuted;
    QAction* actSelUnmuted;
};
