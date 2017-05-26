
#include <anmp.hpp>

#include "PlaylistModel.h"

#include <QBrush>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QDirIterator>
#include <QProgressDialog>
#include <QApplication>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

PlaylistModel::~PlaylistModel()
{
    this->songsToAdd.shutDown=true;

    try
    {
        CLOG(LogLevel_t::Info, "Waiting for song adder thread to finish...");
        std::future_status status = this->songAdderWorker.wait_for(std::chrono::seconds(15));
        if (status == std::future_status::timeout)
        {
            CLOG(LogLevel_t::Fatal, "song adder thread not responding, probably got stuck. Aborting.");
            std::terminate();
        }
    }
    catch(const std::future_error& e)
    {
        // probably no future associated, that's fine here
    }
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
        emit this->UnloadCurrentSong();
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

    return data->hasFormat("text/uri-list");
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
    this->asyncAdd(urls);

    return true;
}

void PlaylistModel::asyncAdd(const QFileInfoList& files)
{
    std::unique_lock<mutex> lck(this->songsToAdd.mtx);
    this->songsToAdd.cv.wait(lck, [this]{return !this->songsToAdd.ready;});

    for(int i=0; i<files.count() && !this->songsToAdd.shutDown; i++)
    {
        QString file = files.at(i).absoluteFilePath();
        QFileInfo info(file);
        
        if(info.isDir())
        {
            QDir dir(file);
            lck.unlock();
            this->asyncAdd(dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::LocaleAware));
            lck.lock();
        }
        else
        {
            this->songsToAdd.queue.push_back(file.toUtf8().constData());
        }
    }
    this->songsToAdd.ready = true;

    if(this->songsToAdd.processed && !this->songsToAdd.shutDown)
    {
        this->songAdderWorker = std::async(launch::async, &PlaylistModel::workerLoop, this);
        this->songsToAdd.processed = false;
    }
    lck.unlock();
    this->songsToAdd.cv.notify_one();
}

void PlaylistModel::asyncAdd(const QStringList& files)
{
    std::unique_lock<mutex> lck(this->songsToAdd.mtx);
    this->songsToAdd.cv.wait(lck, [this]{return !this->songsToAdd.ready;});

    for(int i=0; i<files.count() && !this->songsToAdd.shutDown; i++)
    {
        QString file = files.at(i);
        QFileInfo info(file);
        
        if(info.isDir())
        {
            QDir dir(file);
            lck.unlock();
            this->asyncAdd(dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::LocaleAware));
            lck.lock();
        }
        else
        {
            this->songsToAdd.queue.push_back(file.toUtf8().constData());
        }
    }
    this->songsToAdd.ready = true;

    if(this->songsToAdd.processed && !this->songsToAdd.shutDown)
    {
        this->songAdderWorker = std::async(launch::async, &PlaylistModel::workerLoop, this);
        this->songsToAdd.processed = false;
    }
    lck.unlock();
    this->songsToAdd.cv.notify_one();
}

void PlaylistModel::asyncAdd(const QList<QUrl>& files)
{
    std::unique_lock<mutex> lck(this->songsToAdd.mtx);
    this->songsToAdd.cv.wait(lck, [this]{return !this->songsToAdd.ready;});

    for(int i=0; i<files.count() && !this->songsToAdd.shutDown; i++)
    {        
        QString file = files.at(i).toLocalFile();
        QFileInfo info(file);
        
        if(info.isDir())
        {
            QDir dir(file);
            lck.unlock();
            this->asyncAdd(dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::LocaleAware));
            lck.lock();
        }
        else
        {
            this->songsToAdd.queue.push_back(file.toStdString());
        }
    }
    this->songsToAdd.ready = true;

    if(this->songsToAdd.processed && !this->songsToAdd.shutDown)
    {
        this->songAdderWorker = std::async(launch::async, &PlaylistModel::workerLoop, this);
        this->songsToAdd.processed = false;
    }
    lck.unlock();
    this->songsToAdd.cv.notify_one();
}


void PlaylistModel::workerLoop()
{
    int i=0;
    std::unique_lock<mutex> lck(this->songsToAdd.mtx);
//     this->songsToAdd.processed = false;

    while(!this->songsToAdd.queue.empty() && !this->songsToAdd.shutDown)
    {
        // grep the file at the very front and pop it from queue
        const string s = std::move(this->songsToAdd.queue[0]);
        this->songsToAdd.queue.pop_front();
        const int total = this->songsToAdd.queue.size();

        this->songsToAdd.ready = false;
        // release the lock to do the time intensive work
        lck.unlock();
        // notify waiting threads, so they can continue filling the queue
        this->songsToAdd.cv.notify_all();

        auto start = std::chrono::high_resolution_clock::now();
        i++;
        emit this->SongAdded(QString::fromStdString(s), i, i+total);
        PlaylistFactory::addSong(*this, s);
        auto end = std::chrono::high_resolution_clock::now();

        // if we add songs too fast we keep locking this->mtx (via this->add(Song*)) and thus potentially blocking the UI thread
        std::chrono::milliseconds wait = std::chrono::milliseconds(10);
        std::chrono::milliseconds past = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        wait -= past;
        if(wait > std::chrono::milliseconds(0))
        {
            std::this_thread::sleep_for(wait);
        }

        lck.lock();
        this->songsToAdd.ready = true;
    }

    this->songsToAdd.processed = true;
    this->songsToAdd.ready = false;
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

    this->removeRows(0, Elements);
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
    else if(this->getSong(row) == nullptr)
    {
        return QColor(255,225,0);
    }
    else if(row > 0 && row % 10 == 0)
    {
        return QColor(220,220,220);
    }
    else if(row % 2 == 0)
    {
        return QColor(233,233,233);
    }
    
    return QColor(Qt::white);
}

void PlaylistModel::shuffle(unsigned int start, unsigned int end)
{
    this->beginResetModel();
    Playlist::shuffle(start, end);
    this->endResetModel();
}
