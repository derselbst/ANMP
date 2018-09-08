#pragma once

#include <QTableView>


class QContextMenuEvent;
class QAction;
class MainWindow;

class ChannelConfigView : public QTableView
{
    Q_OBJECT

    public:
    ChannelConfigView(QWidget *parent = 0);
    void Select(Qt::CheckState state);
    void SetContextMenu(MainWindow *wnd, QMenu *menu);

    protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;


    MainWindow *mainWindow;
    QMenu *contextMenu;
};
