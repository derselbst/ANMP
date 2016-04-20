#include "PlaylistModel.h"

#include "Song.h"
#include "Common.h"

#include <QBrush>
#include <sstream>
#include <iomanip>

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractTableModel(parent)
{
} 


int PlaylistModel::rowCount(const QModelIndex & /* parent */) const
{
    return this->queue.size();
}
int PlaylistModel::columnCount(const QModelIndex & /* parent */) const
{
    return 4;
}


#include <iostream>
QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
//       cout << "    DATA " << index.row() << " " << index.column() << role << endl;
    if (!index.isValid() || this->queue.size() <= index.row())
    {
      return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
      Song* songToUse = this->queue[index.row()];
      
      switch(index.column())
      {
    case 0:
      {
          string s="";
          if(songToUse->Metadata.Track != "")
          {
              s += songToUse->Metadata.Track;
              s += " - ";
          }

          if(songToUse->Metadata.Title == "")
          {
              string tmp = songToUse->Filename;
              tmp = basename(tmp.c_str());
              s += tmp;
          }
          else
          {
              s += songToUse->Metadata.Title;
          }

          return QString::fromStdString(s);
      }
    case 1:
        return QString::fromStdString(songToUse->Metadata.Album);
    case 2:
        return QString::fromStdString(songToUse->Metadata.Artist);
    case 3:
      {
          return QString::fromStdString(framesToTimeStr(songToUse->getFrames(),songToUse->Format.SampleRate));
      }
	default:
	  break;
      }
    }
    else if (role == Qt::BackgroundRole)
    {
           int row = index.row();
           QColor color = this->calculateRowColor(row);           
           return QBrush(color);
        }
    return QVariant();
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation==Qt::Horizontal)
  {
    // TODO: Complete Me!
    switch(role)
    {
      case Qt::DisplayRole: // The key data to be rendered in the form of text.
	switch(section)
	{
      case 0:
        return QString("Title");
      case 1:
        return QString("Album");
      case 2:
        return QString("Interpret");
      case 3:
        return QString("Duration");
	  default:
	    return QString("");
	}
	break;
       case Qt::ToolTipRole: // The data displayed in the item's tooltip.
	    break;
       case Qt::WhatsThisRole: // The data displayed for the item in "What's This?" mode.
	    break;
       default:
	 // do nothing special here
	 break;
    }
  }
  else // i.e. orientation==Qt::Vertical
  {
    if(role==Qt::DisplayRole)
    {
      // use one-based counting for rows
      return QString::number(section+1);
    }
  }
  
  return QVariant();
}


bool PlaylistModel::insertRows(int row, int count, const QModelIndex & parent)
{
  if(row>this->rowCount(QModelIndex()))
  {
    return false;
  }

  //notify views and proxy models that a line will be inserted
    beginInsertRows(parent, row, row+(count-1));
  //finish insertion, notify views/models
    endInsertRows();

  return true;
}

bool PlaylistModel::removeRows(int row, int count, const QModelIndex & parent)
{
    if(row+count>this->rowCount(QModelIndex()))
    {
      return false;
    }

    //notify views and proxy models that a line will be inserted
      beginRemoveRows(parent, row, row+(count-1));

      for(int i=row+(count-1); i>=row; i--)
      {
          Playlist::remove(i);
      }

    //finish insertion, notify views/models
      endRemoveRows();

    return true;
}

void PlaylistModel::move(std::deque<Song*>& que, signed int source, unsigned int count, int steps)
{
    if(source < 0 || que.size() < source+count)
    {
        return;
    }
    if(steps<0) // left shift
    {
        rotate(source+steps >= 0 ?
                    std::next(que.begin(),source+steps) :
                    que.begin(),
               std::next(que.begin(),source),
               std::next(que.begin(),source+count+1)
              );

    }
    else if(steps>0) // right shift
    {
        rotate(std::next(que.begin(),source),
               std::next(que.begin(),source+count+1),
               source+count+steps < que.size() ?
                    std::next(que.begin(),source+count+steps+1) :
                    que.end()
              );
    }

}

bool PlaylistModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationRow)
{
     // if source and destination parents are the same, move elements locally
     if(true)
     {
             beginMoveRows(sourceParent, sourceRow, sourceRow+count-1, destinationParent, destinationRow);
             this->move(this->queue, sourceRow, count, destinationRow-sourceRow);
             endMoveRows();
     }
     else
     {
         // otherwise, move the node under the parent
         return false;
     }
    return true;
}

bool PlaylistModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() /*&& index.row() != index.column()*/ /*&& role == Qt::EditRole*/)
    {
      return false;
    }
    
    if(role==Qt::BackgroundRole)
    {
      cout << "EMIT " << index.row() << " " << index.column() << endl;
      // TODO WHY DOES THIS HAS NO FUCKING EFFECT???
      emit(dataChanged(index, index));
    }
    

    // WARNING: the view will only properly be updated if we return true!
    return true;
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
        flags |= Qt::ItemIsEditable;
	
    return flags;
}




void PlaylistModel::add(Song* s)
{
  Playlist::add(s);
  
  this->insertRows(this->rowCount(QModelIndex()), 1);
}

void PlaylistModel::remove(int i)
{
    this->removeRows(i, 1);
}

void PlaylistModel::clear()
{
    const int Elements = this->rowCount(QModelIndex());

    for(int i=0; i<Elements; i++)
    {
        Playlist::remove(0);
        this->removeRows(0, 1);
    }
}

Song* PlaylistModel::setCurrentSong (unsigned int id) 
{
   this->beginResetModel();

  Song* newSong = Playlist::setCurrentSong(id);

   this->endResetModel();

  return newSong;
}
// TODO: acutally smth like this should be done here for setCurrentSong()
// Song* PlaylistModel::setCurrentSong () 
// {
//   int oldSong = this->currentSong;
//   Song* newSong = Playlist::setCurrentSong();
//   
// //     clear color of old song
//   this->setData(this->index(oldSong, 0), this->calculateRowColor(oldSong),Qt::BackgroundRole);
// 
// //     color new song in view
//     this->setData(this->index(this->currentSong, 0), this->calculateRowColor(this->currentSong),Qt::BackgroundRole);
//     
//     return newSong;
// }

QColor PlaylistModel::calculateRowColor(int row) const
{
  if(this->currentSong == row)
  {
    return QColor(255, 0, 0, 127);
  }
  else if(row % 2 == 0)
  {
      return QColor(222,222,222);
  }
  return QColor(Qt::white);
}
