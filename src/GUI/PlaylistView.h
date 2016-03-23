#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QTableView>


class PlaylistView : public QTableView
{
  Q_OBJECT
  
public:
    PlaylistView(QWidget * parent = 0);
    
signals:
  void remove(QModelIndexList&);
  void swap(QModelIndexList&, int);

protected:
    void keyPressEvent(QKeyEvent * event) override;

};


#endif
