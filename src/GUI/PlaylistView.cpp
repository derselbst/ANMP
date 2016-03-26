#include "PlaylistView.h"
#include "IPlaylist.h"
#include "PlaylistModel.h"
#include <QKeyEvent>

PlaylistView::PlaylistView(QWidget * parent)
    : QTableView(parent)
{
} 

// sort ascendingly
bool sortQModelIndexList(QModelIndex i,QModelIndex j)
{
    return i.row()<j.row();
}

void PlaylistView::keyPressEvent(QKeyEvent * event)
{
    int key = event->key();
  switch(key)
  {
    case Qt::Key_Delete:
    {
    QModelIndexList indexList = this->selectionModel()->selectedRows();
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

     case Qt::Key_U:
  {
      QModelIndexList indexList = this->selectionModel()->selectedRows();
      std::sort(indexList.begin(), indexList.end(), sortQModelIndexList);

      PlaylistModel* playlistModel = dynamic_cast<PlaylistModel*>(this->model());
      if(playlistModel!=nullptr)
      {
              playlistModel->moveRows(playlistModel->index(0,0), indexList[0].row(), indexList.size()-1, playlistModel->index(playlistModel->rowCount(QModelIndex())-1, playlistModel->columnCount(QModelIndex())-1), indexList[0].row()-1);
              QModelIndex newIdx = playlistModel->index(indexList[indexList.size()-1].row()-1,indexList[indexList.size()-1].column());
              if(newIdx.isValid())
              {
              this->setCurrentIndex(newIdx);
              }
      }
  }
      break;
  case Qt::CTRL + Qt::Key_Down:
      break;

    default:
      QTableView::keyPressEvent(event);
    break;
  }
}
