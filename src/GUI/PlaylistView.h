#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QTableView>


class PlaylistView : public QTableView
{
  Q_OBJECT
  
public:
    PlaylistView(QWidget * parent = 0);

protected:
    void keyPressEvent(QKeyEvent * event) override;

private:
    void moveItems(int steps);
};


#endif
