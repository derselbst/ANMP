#include "PlaylistModel.h"

#include "Song.h"
#include "Common.h"
#include "PlaylistFactory.h"
#include "mainwindow.h"

#include <QBrush>
#include <QMimeData>
#include <QUrl>
#include <QProgressDialog>
#include <QApplication>
#include <sstream>
#include <iostream>
#include <iomanip>

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}


int PlaylistModel::rowCount(const QModelIndex & /* parent */) const
{
    lock_guard<recursive_mutex> lck(this->mtx);

    return this->queue.size();
}
int PlaylistModel::columnCount(const QModelIndex & /* parent */) const
{
    return 5;
}


QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    lock_guard<recursive_mutex> lck(this->mtx);

//       cout << "    DATA " << index.row() << " " << index.column() << role << endl;
    if (!index.isValid() || this->queue.size() <= index.row())
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        Song* songToUse = this->queue[index.row()];

        if(songToUse == nullptr)
        {
            return QString("---");
        }

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
                s += mybasename(songToUse->Filename);
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
            return QString::fromStdString(songToUse->Metadata.Genre);
        case 4:
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
                return QString("Genre");
            case 4:
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
    lock_guard<recursive_mutex> lck(this->mtx);

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
    // call QObjects parent() explicitly
    // in QT5.1 parent is overridden or hidden or something by QAbstractTableModel, which may break build
    QObject* p = this->QObject::parent();

    // stop playback if the currently playing song is about to be removed
    if(p != nullptr && row <= this->currentSong && this->currentSong <= row+count-1)
    {
        MainWindow* wnd = dynamic_cast<MainWindow*>(p);
        if(wnd != nullptr)
        {
            wnd->stop();
            wnd->player->setCurrentSong(nullptr);
        }
    }

    lock_guard<recursive_mutex> lck(this->mtx);

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

bool PlaylistModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationRow)
{
    lock_guard<recursive_mutex> lck(this->mtx);

    // if source and destination parents are the same, move elements locally
    if(true)
    {
        beginMoveRows(sourceParent, sourceRow, sourceRow+count-1, destinationParent, destinationRow);
        this->move(sourceRow, count, destinationRow-sourceRow);
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

    if (index.isValid())
    {
        flags |= Qt::ItemIsEditable;
        flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }
    else
    {
        flags |= Qt::ItemIsDropEnabled;
    }

    return flags;
}

Qt::DropActions PlaylistModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList PlaylistModel::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list";
    return types;
}

QMimeData* PlaylistModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    foreach (const QModelIndex &index, indexes)
    {
        if (index.isValid())
        {
            QString text = data(index, Qt::DisplayRole).toString();
            stream << text;
        }
    }

    mimeData->setData("application/vnd.text.list", encodedData);
    return mimeData;
}

bool PlaylistModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(parent);

    if (!data->hasFormat("text/uri-list"))
        return false;

    if (column > 0)
        return false;

    return true;
}


bool PlaylistModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    int beginRow;

    if (row != -1)
    {
        beginRow = row;
    }
    else if (parent.isValid())
    {
        beginRow = parent.row();
    }
    else
    {
        beginRow = this->rowCount(QModelIndex());
    }


    QList<QUrl> urls = data->urls();

    MainWindow* wnd = dynamic_cast<MainWindow*>(this->QObject::parent());

    QProgressDialog progress("Adding files...", "Abort", 0, urls.count(), wnd);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

    for(int i=0; !progress.wasCanceled() && i<urls.count(); i++)
    {
        // ten redrawings
        if(i%(static_cast<int>(urls.count()*0.1)+1)==0)
        {
            progress.setValue(i);
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
        }

        PlaylistFactory::addSong(*this, urls.at(i).toLocalFile().toStdString());
        beginRow++;
    }

    return true;
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
    lock_guard<recursive_mutex> lck(this->mtx);

    const int Elements = this->rowCount(QModelIndex());

    for(int i=0; i<Elements; i++)
    {
        Playlist::remove(0);
        this->removeRows(0, 1);
    }
}

Song* PlaylistModel::setCurrentSong (unsigned int id)
{
    lock_guard<recursive_mutex> lck(this->mtx);

    this->beginResetModel();

    Song* newSong = Playlist::setCurrentSong(id);

    this->endResetModel();

    return newSong;
}
// TODO: acutally smth like this should be done here for setCurrentSong()
// Song* PlaylistModel::setCurrentSong ()
// {
//     lock_guard<recursive_mutex> lck(this->mtx);
//
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
    lock_guard<recursive_mutex> lck(this->mtx);

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