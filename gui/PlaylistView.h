#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QTableView>

class QKeyEvent;
class SongInspector;

class PlaylistView : public QTableView
{
    Q_OBJECT

public:
    PlaylistView(QWidget * parent = 0);

protected:
    void keyPressEvent(QKeyEvent * event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    SongInspector* inspectorView = nullptr;

    void moveItems(int steps);

protected slots:
    void showInspector();
};


#endif
