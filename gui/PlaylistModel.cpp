
#include <anmp.hpp>

#include "Playlist.h"
#include "PlaylistModel.h"

#include <QApplication>
#include <QBrush>
#include <QDirIterator>
#include <QFileInfo>
#include <QMimeData>
#include <QProgressDialog>
#include <QUrl>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

PlaylistModel::PlaylistModel(Playlist *playlist, QObject *parent)
: QAbstractTableModel(parent),
  playlist(playlist)
{
}

PlaylistModel::~PlaylistModel()
{
    this->songsToAdd.shutDown = true;
    this->songsToAdd.queue.clear();

    if(this->songAdderWorker.valid())
    {
        CLOG(LogLevel_t::Info, "Waiting for song adder thread to finish...");
        std::future_status status = this->songAdderWorker.wait_for(std::chrono::seconds(15));
        if (status == std::future_status::timeout)
        {
            CLOG(LogLevel_t::Fatal, "song adder thread not responding, probably got stuck. Terminating.");
            std::terminate();
        }
    }
}

int PlaylistModel::rowCount(const QModelIndex & /* parent */) const
{
    return this->playlist->size();
}

int PlaylistModel::columnCount(const QModelIndex & /* parent */) const
{
    return 5;
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    //       cout << "    DATA " << index.row() << " " << index.column() << role << endl;
    if (index.isValid() && this->rowCount(index) > index.row())
    {
        if (role == Qt::DisplayRole)
        {
            Song *songToUse = this->playlist->getSong(index.row());

            if (songToUse == nullptr)
            {
                return QString("---");
            }

            switch (index.column())
            {
                case 0:
                {
                    string s = "";
                    if (songToUse->Metadata.Track != "")
                    {
                        s += songToUse->Metadata.Track;
                        s += " - ";
                    }

                    if (songToUse->Metadata.Title == "")
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
                    return QString::fromStdString(framesToTimeStr(songToUse->getFrames(), songToUse->Format.SampleRate));
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
        else if (role == Qt::TextAlignmentRole)
        {
            switch (index.column())
            {
                case 0:
                    return (Qt::AlignLeft + Qt::AlignVCenter);

                default:
                    return Qt::AlignCenter;
            }
        }
    }
    return QVariant();
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal)
    {
        // TODO: Complete Me!
        switch (role)
        {
            case Qt::DisplayRole: // The key data to be rendered in the form of text.
                switch (section)
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
        if (role == Qt::DisplayRole)
        {
            // use one-based counting for rows
            return QString::number(section + 1);
        }
    }

    return QVariant();
}


bool PlaylistModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row > this->rowCount(QModelIndex()))
    {
        return false;
    }

    //notify views and proxy models that a line will be inserted
    beginInsertRows(parent, row, row + (count - 1));
    //finish insertion, notify views/models
    endInsertRows();

    return true;
}

bool PlaylistModel::removeRows(int row, int count, const QModelIndex &parent)
{
    int curentId = this->playlist->getCurrentSongId();

    if (row + count > this->rowCount(QModelIndex()))
    {
        return false;
    }

    // stop playback if the currently playing song is about to be removed
    if (row <= curentId && curentId <= row + count - 1)
    {
        emit this->UnloadCurrentSong();
    }

    //notify views and proxy models that a line will be removed
    beginRemoveRows(parent, row, row + (count - 1));

    for (int i = row + (count - 1); i >= row; i--)
    {
        this->playlist->remove(i);
    }

    //finish removal, notify views/models
    endRemoveRows();

    this->slotCurrentSongChanged(nullptr);

    return true;
}

bool PlaylistModel::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationRow)
{
    // if source and destination parents are the same, move elements locally
    if (true)
    {
        beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationRow);
        this->playlist->move(sourceRow, count, destinationRow - sourceRow);
        endMoveRows();
    }
    else
    {
        // otherwise, move the node under the parent
        return false;
    }
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

QMimeData *PlaylistModel::mimeData(const QModelIndexList &indexes) const
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

bool PlaylistModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
    {
        return false;
    }

    if (action == Qt::IgnoreAction)
    {
        return true;
    }

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
    this->asyncAdd(urls);

    return true;
}

QString PlaylistModel::__toQString(const QFileInfo &fi)
{
    return fi.absoluteFilePath();
}

QString PlaylistModel::__toQString(const QString &str)
{
    return str;
}

QString PlaylistModel::__toQString(const QUrl &url)
{
    return url.toLocalFile();
}


void PlaylistModel::workerLoop()
{
    int i = 0;
    std::vector<Song*> freshSongs;
    
    std::unique_lock<std::recursive_mutex> lck(this->songsToAdd.mtx);
    this->songsToAdd.ready = true;
    
    freshSongs.reserve(this->songsToAdd.queue.size()+1);
    while (!this->songsToAdd.queue.empty() && !this->songsToAdd.shutDown)
    {
        // grep the file at the very front and pop it from queue
        const QString s = std::move(this->songsToAdd.queue[0]);
        this->songsToAdd.queue.pop_front();
        const int total = this->songsToAdd.queue.size();

        this->songsToAdd.ready = false;
        // release the lock to do the time intensive work
        lck.unlock();
        // notify waiting threads, so they can continue filling the queue
        this->songsToAdd.cv.notify_all();

        freshSongs.clear();
        PlaylistFactory::addSong(freshSongs, s.toStdString());
        int songsAdded = freshSongs.size();
        std::this_thread::yield();

        if (songsAdded > 0)
        {
            auto songAddedInRow = this->playlist->add(freshSongs[0]) + 1;
            for(decltype(songsAdded) j=1; j<songsAdded; j++)
            {
                this->playlist->add(freshSongs[j]);
            }
            QMetaObject::invokeMethod(this, "insertRows", Qt::QueuedConnection, Q_ARG(int, songAddedInRow), Q_ARG(int, songsAdded));
            emit this->SongAdded(s, i, i + total);
        }
        i++;
        
        lck.lock();
        this->songsToAdd.ready = true;
    }

    this->songsToAdd.processed = true;
    this->songsToAdd.ready = false;
}

void PlaylistModel::clear()
{
    {
        std::lock_guard<std::recursive_mutex> lck(this->songsToAdd.mtx);
        this->songsToAdd.queue.clear();
    }

    if(this->songAdderWorker.valid())
    {
        std::future_status status = this->songAdderWorker.wait_for(std::chrono::seconds(5));
        if (status == std::future_status::timeout)
        {
            CLOG(LogLevel_t::Fatal, "song adder thread not responding, probably got stuck. Terminating.");
            std::terminate();
        }
    }

    this->beginResetModel();
    this->playlist->clear();
    this->endResetModel();
}

void PlaylistModel::slotCurrentSongChanged(const Song *)
{
    const int oldSongId = this->oldSongId;
    const int newSongId = this->playlist->getCurrentSongId();

    if (oldSongId != newSongId)
    {
        const int lastCol = this->columnCount(QModelIndex()) - 1;

        emit(dataChanged(this->index(oldSongId, 0), this->index(oldSongId, lastCol)));
        emit(dataChanged(this->index(newSongId, 0), this->index(newSongId, lastCol)));

        this->oldSongId = newSongId;
    }
}

QColor PlaylistModel::calculateRowColor(int row) const
{
    if (this->playlist->getSong(row) == nullptr)
    {
        return QColor(255, 0, 0, 127);
    }
    else if (this->playlist->getCurrentSongId() == row)
    {
        return QColor(255, 225, 0);
    }
    else if (row > 0 && row % 10 == 0)
    {
        return QColor(220, 220, 220);
    }
    else if (row % 2 == 0)
    {
        return QColor(233, 233, 233);
    }

    return QColor(Qt::white);
}

void PlaylistModel::shuffle(size_t start, size_t end)
{
    this->beginResetModel();
    this->playlist->shuffle(start, end);
    this->endResetModel();
}
