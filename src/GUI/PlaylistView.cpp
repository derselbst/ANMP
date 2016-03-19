#include "PlaylistView.h"
#include <QKeyEvent>

PlaylistView::PlaylistView(QWidget * parent)
    : QTableView(parent)
{
} 

void PlaylistView::keyPressEvent(QKeyEvent * event)
{
  switch(event->key())
  {
    case Qt::Key_Delete:
    {
    QModelIndexList indexList = this->selectionModel()->selectedRows();
      emit remove(indexList);
    }
      break;
      
    default:
      QTableView::keyPressEvent(event);
    break;
  }
}
