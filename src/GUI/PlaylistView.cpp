#include "PlaylistView.h"
#include "IPlaylist.h"
#include <QKeyEvent>

PlaylistView::PlaylistView(QWidget * parent)
    : QTableView(parent)
{
} 

bool sortQModelIndexList(QModelIndex i,QModelIndex j)
{
    return i.row()>j.row();
}

void PlaylistView::keyPressEvent(QKeyEvent * event)
{
  switch(event->key())
  {
    case Qt::Key_Delete:
    {
    QModelIndexList indexList = this->selectionModel()->selectedRows();
//      emit remove(indexList);
    int lastRow=0;
  for(QModelIndexList::const_iterator i=indexList.cbegin(); i!=indexList.cend(); ++i)
  {
    if(i->isValid())
    {
      IPlaylist* playlistModel = dynamic_cast<IPlaylist*>(this->model());
      if(playlistModel!=nullptr)
      {
              playlistModel->remove(i->row()-lastRow);
      lastRow=i->row();
      }
   }
  }
    }
      break;

     case Qt::CTRL + Qt::Key_Up:
//      QModelIndexList indexList = this->selectionModel()->selectedRows();
//      std::sort(indexList.begin(), indexList.end(), sortQModelIndexList());
//      int swapItem = indexList[0].row()-1;
//      if(swapItem>=0)
//      {
//          emit swap(indexList, swapItem);
//          this->model();
//      }
      break;
  case Qt::CTRL + Qt::Key_Down:
      break;

    default:
      QTableView::keyPressEvent(event);
    break;
  }
}
