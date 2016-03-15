#include "PlaylistModel.h"

#include "Song.h"

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
    return 2;
}


QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
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
	  return QString::fromStdString(songToUse->Filename);
    case 1:
	  return QString::fromStdString(songToUse->Metadata.Artist);
	default:
	  break;
      }
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
	    return QString("Filename");
      case 1:
	    return QString("Interpret");
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
    if(row>this->rowCount(QModelIndex()))
    {
      return false;
    }

    //notify views and proxy models that a line will be inserted
      beginRemoveRows(parent, row, row+(count-1));
    //finish insertion, notify views/models
      endRemoveRows();

    return true;
}



bool PlaylistModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
//     if (index.isValid() /*&& index.row() != index.column()*/ /*&& role == Qt::EditRole*/)
//     {
//       
//         int offset = offsetOf(index.row(), index.column());
//         distances[offset] = value.toInt();
// 
//         QModelIndex transposedIndex = createIndex(index.column(),
//                                                   index.row());
//         emit dataChanged(index, index);
//         emit dataChanged(transposedIndex, transposedIndex);
//         return true;
//     }
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
    Playlist::remove(i);

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
