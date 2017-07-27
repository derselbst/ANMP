#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QModelIndex>
#include <QAbstractTableModel>
#include <QColor>

#include <mutex>
#include <condition_variable>
#include <future>

#include "Playlist.h"

class Song;
class QFileInfo;

class PlaylistModel : public QAbstractTableModel, public Playlist
{
    Q_OBJECT

public:
    PlaylistModel(QObject *parent = 0);
    ~PlaylistModel() override;

    // *** MODEL READ SUPPORT ***
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;


    // *** MODEL WRITE SUPPORT ***
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationRow) override;

    // *** DRAG + DROP ***
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    size_t add (Song* song) override;

    void remove (size_t i) override;

    void clear() override;

    Song* setCurrentSong (size_t id) override;

    void shuffle(size_t, size_t) override;


    template<typename T>
    void asyncAdd(const QList<T>&);

signals:
    void SongAdded(QString file, int cur, int total);

    // emitted, if the song currently played back gets deleted
    void UnloadCurrentSong();

private:
    struct
    {
        // a queue to be filled with filepaths for songs that shall be added
        std::deque<string> queue;

        // mutex for that queue
        mutable mutex mtx;

        // cond var for synchronizing the worker thread and queue filler threads
        std::condition_variable cv;

        // predicate variable for cv used by the queue filler threads
        // true: the worker thread is currently owner of the mutex, i.e. it's messing around the the queue
        // false: not owner of mtx, i.e. other threads can fill the queue as soon as they get notified via the condition variable
        bool ready = false;

        // true: the whole queue has been process, i.e. no worker thread is currently running
        // false: worker thread is running and queue contains data to be processed
        bool processed = true;

        // if the application is supposed to be shut down
        bool shutDown = false;
    } songsToAdd;

    void workerLoop();
    std::future<void> songAdderWorker;

    QColor calculateRowColor(int row) const;
    
    static QString __toQString(const QFileInfo& fi);
    static QString __toQString(const QString& str);
    static QString __toQString(const QUrl& url);

};

#include <QDir>
template<typename T>
void PlaylistModel::asyncAdd(const QList<T>& files)
{
    std::unique_lock<mutex> lck(this->songsToAdd.mtx);
    this->songsToAdd.cv.wait(lck, [this]{return !this->songsToAdd.ready;});

    for(int i=0; i<files.count() && !this->songsToAdd.shutDown; i++)
    {        
        QString file = this->__toQString(files.at(i));
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

#endif
