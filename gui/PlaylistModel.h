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

    void add (Song* song) override;

    void remove (int i) override;

    void clear() override;

    Song* setCurrentSong (unsigned int id) override;

    void shuffle(unsigned int, unsigned int) override;



    void asyncAdd(const QStringList& files);
    void asyncAdd(const QList<QUrl>& files);

signals:
    void SongAdded(QString file, int cur, int total);

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

};


#endif
