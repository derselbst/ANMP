#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractTableModel>
#include <QColor>
#include <QModelIndex>
#include <QCoreApplication>
#include <QDir>

#include <condition_variable>
#include <deque>
#include <future>
#include <mutex>
#include <string>

class Playlist;
class Song;
class QFileInfo;

class PlaylistModel : public QAbstractTableModel
{
    Q_OBJECT

    public:
    PlaylistModel(Playlist *playlist, QObject *parent = 0);
    ~PlaylistModel() override;

    // *** MODEL READ SUPPORT ***
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;


    // *** MODEL WRITE SUPPORT ***
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationRow) override;

    // *** DRAG + DROP ***
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    void clear();
    void shuffle(size_t, size_t);
    const Playlist *getPlaylist()
    {
        return this->playlist;
    };


    template<typename T>
    void asyncAdd(const QList<T> &);
    
    void SlotCurrentSongChanged(const Song *s);

    signals:
    void SongAdded(QString file, int cur, int total);

    // emitted, if the song currently played back gets deleted
    void UnloadCurrentSong();

    public slots:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    private:
    Playlist *playlist;
    int oldSongId = 0;

    struct
    {
        // a queue to be filled with filepaths for songs that shall be added
        std::deque<QString> queue;

        // mutex for that queue
        mutable std::recursive_mutex mtx;

        // cond var for synchronizing the worker thread and queue filler threads
        std::condition_variable_any cv;

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

    static QString __toQString(const QFileInfo &fi);
    static QString __toQString(const QString &str);
    static QString __toQString(const QUrl &url);
};

template<typename T>
void PlaylistModel::asyncAdd(const QList<T> &files)
{
    std::unique_lock<std::recursive_mutex> lck(this->songsToAdd.mtx);
    this->songsToAdd.cv.wait(lck, [this] { return !this->songsToAdd.ready; });

    for (int i = 0; i < files.count() && !this->songsToAdd.shutDown; i++)
    {
        QString file = this->__toQString(files.at(i));
        QFileInfo info(file);

        if (info.isDir())
        {
            QDir dir(file);
            this->asyncAdd(dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::LocaleAware));
        }
        else
        {
            this->songsToAdd.queue.emplace_back(std::move(file));
        }
        
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 1/*ms*/);
    }

    if (this->songsToAdd.processed && !this->songsToAdd.shutDown)
    {
        this->songsToAdd.processed = false;
        this->songAdderWorker = std::async(std::launch::async, &PlaylistModel::workerLoop, this);
    }
    lck.unlock();
    this->songsToAdd.cv.notify_one();
}

#endif
