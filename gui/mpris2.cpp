#ifdef USE_DBUS

#include "mpris2.h"

#include "Playlist.h"
#include "PlaylistModel.h"
#include "Player.h"
#include "Song.h"
#include "mainwindow.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QFileInfo>
#include <QUrl>

static qlonglong framesToUsec(frame_t frames, uint32_t sampleRate)
{
    return sampleRate == 0 ? 0 : static_cast<qlonglong>((frames * 1000000LL) / sampleRate);
}

Mpris2::Mpris2(MainWindow *window, Player *player, Playlist *playlist, PlaylistModel *playlistModel, QObject *parent)
: QObject(parent),
  window(window),
  player(player),
  playlist(playlist),
  playlistModel(playlistModel)
{
    qRegisterMetaType<QList<QDBusObjectPath>>("QList<QDBusObjectPath>");
    qDBusRegisterMetaType<QList<QDBusObjectPath>>();

    this->currentMetadata = this->metadataForIndex(this->playlist->getCurrentSongId());

    this->rootAdaptor = new MprisRootAdaptor(this);
    this->playerAdaptor = new MprisPlayerAdaptor(this);
    this->trackAdaptor = new MprisTrackListAdaptor(this);

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected())
    {
        bus.registerObject(QString::fromUtf8(ObjectPath), this);
        bus.registerService(QString::fromUtf8(ServiceName));
    }

    this->refreshTrackList();

    QObject::connect(this->playlistModel, &QAbstractItemModel::rowsInserted, this, &Mpris2::refreshTrackList);
    QObject::connect(this->playlistModel, &QAbstractItemModel::rowsRemoved, this, &Mpris2::refreshTrackList);
    QObject::connect(this->playlistModel, &QAbstractItemModel::modelReset, this, &Mpris2::refreshTrackList);
    QObject::connect(this->playlistModel, &PlaylistModel::SongAdded, this, &Mpris2::onSongAdded);
}

Mpris2::~Mpris2()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (bus.isConnected())
    {
        bus.unregisterObject(QString::fromUtf8(ObjectPath));
        bus.unregisterService(QString::fromUtf8(ServiceName));
    }
}

void Mpris2::emitPropertiesChanged(const QString &iface, const QVariantMap &changed, const QStringList &invalidated)
{
    if (changed.isEmpty() && invalidated.isEmpty())
    {
        return;
    }
    QDBusMessage msg = QDBusMessage::createSignal(QString::fromUtf8(ObjectPath),
                                                  QStringLiteral("org.freedesktop.DBus.Properties"),
                                                  QStringLiteral("PropertiesChanged"));
    msg << iface << changed << invalidated;
    QDBusConnection::sessionBus().send(msg);
}

void Mpris2::updatePlaybackStatus(bool isPlaying)
{
    QString newState = isPlaying ? QStringLiteral("Playing") : QStringLiteral("Paused");
    if (!this->playlist || this->playlist->size() == 0)
    {
        newState = QStringLiteral("Stopped");
    }

    if (newState != this->playbackState)
    {
        this->playbackState = newState;
        QVariantMap changed;
        changed.insert(QStringLiteral("PlaybackStatus"), this->playbackState);
        changed.insert(QStringLiteral("CanPlay"), this->canPlay());
        changed.insert(QStringLiteral("CanPause"), this->canPause());
        changed.insert(QStringLiteral("CanSeek"), this->canSeek());
        this->emitPropertiesChanged(QString::fromUtf8(PlayerInterface), changed);
    }
}

void Mpris2::updatePosition(frame_t frame)
{
    this->positionUsec = framesToUsec(frame, this->currentSampleRate);
}

void Mpris2::emitSeeked(qlonglong posUsec)
{
    this->positionUsec = posUsec;
    emit this->playerAdaptor->Seeked(posUsec);
}

void Mpris2::updateCurrentSong(const Song *song)
{
    Q_UNUSED(song);
    const Song *cur = this->playlist->getCurrentSong();
    this->currentSampleRate = (cur != nullptr) ? cur->Format.SampleRate : 0;
    this->currentMetadata = this->metadataForIndex(this->playlist->getCurrentSongId());
    this->positionUsec = 0;

    QVariantMap changed;
    changed.insert(QStringLiteral("Metadata"), this->currentMetadata);
    changed.insert(QStringLiteral("CanPlay"), this->canPlay());
    changed.insert(QStringLiteral("CanPause"), this->canPause());
    changed.insert(QStringLiteral("CanSeek"), this->canSeek());
    changed.insert(QStringLiteral("CanGoNext"), this->canGoNext());
    changed.insert(QStringLiteral("CanGoPrevious"), this->canGoPrevious());
    this->emitPropertiesChanged(QString::fromUtf8(PlayerInterface), changed);
    emit this->trackAdaptor->TrackMetadataChanged(this->currentMetadata, this->currentTrackId());
}

void Mpris2::refreshTrackList()
{
    QList<QDBusObjectPath> ids = this->trackIds();
    QVariantMap changed;
    changed.insert(QStringLiteral("Tracks"), QVariant::fromValue(ids));
    this->emitPropertiesChanged(QString::fromUtf8(TrackListInterface), changed);
    emit this->trackAdaptor->TrackListReplaced(ids, this->currentTrackId());
}

QString Mpris2::identity() const
{
    return QStringLiteral("ANMP");
}

QString Mpris2::desktopEntry() const
{
    return QStringLiteral("anmp");
}

QStringList Mpris2::supportedUriSchemes() const
{
    return QStringList() << QStringLiteral("file");
}

QStringList Mpris2::supportedMimeTypes() const
{
    return QStringList();
}

bool Mpris2::canQuit() const
{
    return true;
}

bool Mpris2::canRaise() const
{
    return true;
}

bool Mpris2::hasTrackList() const
{
    return true;
}

bool Mpris2::fullscreen() const
{
    return this->fullscreenState;
}

void Mpris2::setFullscreen(bool enabled)
{
    if (this->fullscreenState != enabled)
    {
        this->fullscreenState = enabled;
        QVariantMap changed;
        changed.insert(QStringLiteral("Fullscreen"), this->fullscreenState);
        this->emitPropertiesChanged(QString::fromUtf8(RootInterface), changed);
    }
}

bool Mpris2::canSetFullscreen() const
{
    return false;
}

QString Mpris2::playbackStatus() const
{
    return this->playbackState;
}

QString Mpris2::loopStatus() const
{
    return this->loopState;
}

void Mpris2::setLoopStatus(const QString &status)
{
    if (status == QStringLiteral("None") || status == QStringLiteral("Track") || status == QStringLiteral("Playlist"))
    {
        if (this->loopState != status)
        {
            this->loopState = status;
            QVariantMap changed;
            changed.insert(QStringLiteral("LoopStatus"), this->loopState);
            this->emitPropertiesChanged(QString::fromUtf8(PlayerInterface), changed);
        }
    }
}

double Mpris2::rate() const
{
    return this->rateState;
}

void Mpris2::setRate(double r)
{
    if (this->rateState != r)
    {
        this->rateState = r;
        QVariantMap changed;
        changed.insert(QStringLiteral("Rate"), this->rateState);
        this->emitPropertiesChanged(QString::fromUtf8(PlayerInterface), changed);
    }
}

bool Mpris2::shuffle() const
{
    return this->shuffleState;
}

void Mpris2::setShuffle(bool enabled)
{
    if (this->shuffleState != enabled)
    {
        this->shuffleState = enabled;
        QVariantMap changed;
        changed.insert(QStringLiteral("Shuffle"), this->shuffleState);
        this->emitPropertiesChanged(QString::fromUtf8(PlayerInterface), changed);
    }
}

double Mpris2::volume() const
{
    return this->volumeState;
}

void Mpris2::setVolume(double vol)
{
    if (vol < 0.0)
    {
        return;
    }
    if (this->volumeState != vol)
    {
        this->volumeState = vol;
        QVariantMap changed;
        changed.insert(QStringLiteral("Volume"), this->volumeState);
        this->emitPropertiesChanged(QString::fromUtf8(PlayerInterface), changed);
    }
}

qlonglong Mpris2::position() const
{
    return this->positionUsec;
}

double Mpris2::minimumRate() const
{
    return 1.0;
}

double Mpris2::maximumRate() const
{
    return 1.0;
}

bool Mpris2::canGoNext() const
{
    return this->playlist && this->playlist->size() > 1;
}

bool Mpris2::canGoPrevious() const
{
    return this->playlist && this->playlist->size() > 1;
}

bool Mpris2::canPlay() const
{
    return this->playlist && this->playlist->size() > 0;
}

bool Mpris2::canPause() const
{
    return this->player && this->player->IsPlaying();
}

bool Mpris2::canSeek() const
{
    return this->player && this->player->IsSeekingPossible();
}

bool Mpris2::canControl() const
{
    return true;
}

QVariantMap Mpris2::metadata() const
{
    return this->currentMetadata;
}

QList<QDBusObjectPath> Mpris2::trackIds() const
{
    QList<QDBusObjectPath> ids;
    if (!this->playlist)
    {
        return ids;
    }

    size_t sz = this->playlist->size();
    ids.reserve(static_cast<int>(sz));
    for (size_t i = 0; i < sz; ++i)
    {
        ids.append(QDBusObjectPath(QStringLiteral("/org/mpris/MediaPlayer2/TrackList/%1").arg(static_cast<qulonglong>(i))));
    }
    return ids;
}

QDBusObjectPath Mpris2::currentTrackId() const
{
    if (!this->playlist || this->playlist->size() == 0)
    {
        return QDBusObjectPath(QStringLiteral("/org/mpris/MediaPlayer2/TrackList/0"));
    }
    const qulonglong idx = static_cast<qulonglong>(this->playlist->getCurrentSongId());
    return QDBusObjectPath(QStringLiteral("/org/mpris/MediaPlayer2/TrackList/%1").arg(idx));
}

int Mpris2::indexForTrackId(const QDBusObjectPath &trackId) const
{
    const QString prefix = QStringLiteral("/org/mpris/MediaPlayer2/TrackList/");
    QString path = trackId.path();
    if (!path.startsWith(prefix))
    {
        return -1;
    }
    bool ok = false;
    int idx = path.mid(prefix.length()).toInt(&ok);
    if (!ok)
    {
        return -1;
    }
    return idx;
}

QVariantMap Mpris2::metadataForIndex(size_t index) const
{
    QVariantMap map;
    if (!this->playlist || index >= this->playlist->size())
    {
        return map;
    }

    Song *s = this->playlist->getSong(index);
    QDBusObjectPath id(QStringLiteral("/org/mpris/MediaPlayer2/TrackList/%1").arg(static_cast<qulonglong>(index)));
    map.insert(QStringLiteral("mpris:trackid"), QVariant::fromValue(id));
    if (s == nullptr)
    {
        return map;
    }

    if (s->Format.SampleRate != 0)
    {
        map.insert(QStringLiteral("mpris:length"), framesToUsec(s->getFrames(), s->Format.SampleRate));
    }

    if (!s->Metadata.Title.empty())
    {
        map.insert(QStringLiteral("xesam:title"), QString::fromStdString(s->Metadata.Title));
    }
    if (!s->Metadata.Album.empty())
    {
        map.insert(QStringLiteral("xesam:album"), QString::fromStdString(s->Metadata.Album));
    }
    if (!s->Metadata.Artist.empty())
    {
        map.insert(QStringLiteral("xesam:artist"), QStringList() << QString::fromStdString(s->Metadata.Artist));
    }
    if (!s->Metadata.Genre.empty())
    {
        map.insert(QStringLiteral("xesam:genre"), QStringList() << QString::fromStdString(s->Metadata.Genre));
    }
    if (!s->Metadata.Comment.empty())
    {
        map.insert(QStringLiteral("xesam:comment"), QStringList() << QString::fromStdString(s->Metadata.Comment));
    }
    if (!s->Filename.empty())
    {
        map.insert(QStringLiteral("xesam:url"), QUrl::fromLocalFile(QString::fromStdString(s->Filename)).toString());
    }
    if (!s->Metadata.Track.empty())
    {
        bool ok = false;
        int trackNo = QString::fromStdString(s->Metadata.Track).toInt(&ok);
        if (ok)
        {
            map.insert(QStringLiteral("xesam:trackNumber"), trackNo);
        }
    }
    if (!s->Metadata.Composer.empty())
    {
        map.insert(QStringLiteral("xesam:composer"), QStringList() << QString::fromStdString(s->Metadata.Composer));
    }
    return map;
}

void Mpris2::raiseMainwindow()
{
    if (this->window)
    {
        this->window->show();
        this->window->raise();
        this->window->activateWindow();
    }
}

void Mpris2::quitApplication()
{
    QCoreApplication::quit();
}

void Mpris2::next()
{
    if (this->window)
    {
        this->window->Next();
    }
}

void Mpris2::previous()
{
    if (this->window)
    {
        this->window->Previous();
    }
}

void Mpris2::pause()
{
    if (this->window)
    {
        this->window->Pause();
    }
}

void Mpris2::playPause()
{
    if (this->window)
    {
        this->window->TogglePlayPause();
    }
}

void Mpris2::stop()
{
    if (this->window)
    {
        this->window->Stop();
    }
}

void Mpris2::play()
{
    if (this->window)
    {
        this->window->Play();
    }
}

void Mpris2::seek(qlonglong offsetUsec)
{
    if (this->player == nullptr || this->currentSampleRate == 0)
    {
        return;
    }
    qlonglong newPos = this->positionUsec + offsetUsec;
    if (newPos < 0)
    {
        newPos = 0;
    }
    frame_t frame = static_cast<frame_t>((newPos * this->currentSampleRate) / 1000000LL);
    this->player->seekTo(frame);
    this->emitSeeked(framesToUsec(frame, this->currentSampleRate));
}

void Mpris2::setPosition(const QDBusObjectPath &trackId, qlonglong posUsec)
{
    if (this->player == nullptr || this->currentSampleRate == 0)
    {
        return;
    }
    int idx = this->indexForTrackId(trackId);
    if (idx < 0 || static_cast<size_t>(idx) != this->playlist->getCurrentSongId())
    {
        return;
    }
    if (posUsec < 0)
    {
        posUsec = 0;
    }
    frame_t frame = static_cast<frame_t>((posUsec * this->currentSampleRate) / 1000000LL);
    this->player->seekTo(frame);
    this->emitSeeked(framesToUsec(frame, this->currentSampleRate));
}

void Mpris2::openUri(const QString &uri)
{
    this->addTrack(uri, QDBusObjectPath(QStringLiteral("/")), true);
}

QVariantList Mpris2::getTracksMetadata(const QList<QDBusObjectPath> &tracks) const
{
    QVariantList lst;
    for (const QDBusObjectPath &id : tracks)
    {
        const int idx = this->indexForTrackId(id);
        if (idx < 0)
        {
            lst << QVariantMap();
        }
        else
        {
            lst << this->metadataForIndex(static_cast<size_t>(idx));
        }
    }
    return lst;
}

void Mpris2::addTrack(const QString &uri, const QDBusObjectPath &afterTrack, bool setAsCurrent)
{
    if (this->playlistModel == nullptr)
    {
        return;
    }
    int insertPos = -1;
    const int afterIdx = this->indexForTrackId(afterTrack);
    if (afterIdx >= 0)
    {
        insertPos = afterIdx + 1;
    }
    this->pendingInsertPos = insertPos;

    QUrl url(uri);
    QString file = url.isLocalFile() ? url.toLocalFile() : uri;
    if (file.isEmpty())
    {
        return;
    }

    QList<QString> files;
    files << QFileInfo(file).absoluteFilePath();
    if (setAsCurrent)
    {
        this->pendingSetCurrentPath = QFileInfo(file).absoluteFilePath();
    }
    this->playlistModel->asyncAdd(files);
}

void Mpris2::removeTrack(const QDBusObjectPath &trackId)
{
    int idx = this->indexForTrackId(trackId);
    if (idx < 0 || this->playlist == nullptr)
    {
        return;
    }
    if (static_cast<size_t>(idx) >= this->playlist->size())
    {
        return;
    }

    this->playlistModel->removeRows(idx, 1, QModelIndex());
    emit this->trackAdaptor->TrackRemoved(trackId);
    this->refreshTrackList();
}

void Mpris2::goTo(const QDBusObjectPath &trackId)
{
    int idx = this->indexForTrackId(trackId);
    if (idx < 0 || this->playlist == nullptr)
    {
        return;
    }
    const size_t target = static_cast<size_t>(idx);
    if (target >= this->playlist->size())
    {
        return;
    }

    bool wasPlaying = this->player && this->player->IsPlaying();
    if (this->window)
    {
        this->window->Stop();
    }
    this->player->setCurrentSong(this->playlist->setCurrentSong(target));
    this->updateCurrentSong(this->playlist->getCurrentSong());
    if (wasPlaying)
    {
        this->play();
    }
}

void Mpris2::onSongAdded(const QString &file, int current, int total)
{
    Q_UNUSED(current);
    Q_UNUSED(total);
    int destRowRequest = this->pendingInsertPos;
    this->pendingInsertPos = -1;
    int newRow = -1;
    if (this->playlistModel)
    {
        const int rowCount = this->playlistModel->rowCount(QModelIndex());
        if (rowCount > 0)
        {
            newRow = rowCount - 1;
            int destRow = destRowRequest;
            if (destRow >= 0 && destRow < rowCount && destRow != newRow)
            {
                // destinationRow is interpreted after removal only if destRow > sourceRow; here sourceRow is the last row
                this->playlistModel->moveRows(QModelIndex(), newRow, 1, QModelIndex(), destRow);
                newRow = destRow;
            }
        }
    }

    this->refreshTrackList();
    if (this->playlist && this->playlist->size() > 0 && newRow >= 0)
    {
        QDBusObjectPath afterPath(QStringLiteral("/"));
        if (newRow > 0)
        {
            afterPath = QDBusObjectPath(QStringLiteral("/org/mpris/MediaPlayer2/TrackList/%1").arg(static_cast<qulonglong>(newRow - 1)));
        }
        emit this->trackAdaptor->TrackAdded(this->metadataForIndex(static_cast<size_t>(newRow)), afterPath);
    }
    if (this->pendingSetCurrentPath.isEmpty())
    {
        return;
    }

    QFileInfo added(file);
    if (added.absoluteFilePath() == this->pendingSetCurrentPath)
    {
        if (this->playlist && this->playlist->size() > 0)
        {
            const size_t newIdx = newRow >= 0 ? static_cast<size_t>(newRow) : (this->playlist->size() - 1);
            this->playlist->setCurrentSong(newIdx);
            this->player->setCurrentSong(this->playlist->getCurrentSong());
            this->pendingSetCurrentPath.clear();
            this->updateCurrentSong(this->playlist->getCurrentSong());
        }
    }
}

// ---------------- Root adaptor ----------------

MprisRootAdaptor::MprisRootAdaptor(Mpris2 *parent)
: QDBusAbstractAdaptor(parent),
  mpris(parent)
{
    this->setAutoRelaySignals(true);
}

bool MprisRootAdaptor::canQuit() const { return this->mpris->canQuit(); }
bool MprisRootAdaptor::canRaise() const { return this->mpris->canRaise(); }
bool MprisRootAdaptor::hasTrackList() const { return this->mpris->hasTrackList(); }
QString MprisRootAdaptor::identity() const { return this->mpris->identity(); }
QString MprisRootAdaptor::desktopEntry() const { return this->mpris->desktopEntry(); }
QStringList MprisRootAdaptor::supportedUriSchemes() const { return this->mpris->supportedUriSchemes(); }
QStringList MprisRootAdaptor::supportedMimeTypes() const { return this->mpris->supportedMimeTypes(); }
bool MprisRootAdaptor::fullscreen() const { return this->mpris->fullscreen(); }
void MprisRootAdaptor::setFullscreen(bool enabled) { this->mpris->setFullscreen(enabled); }
bool MprisRootAdaptor::canSetFullscreen() const { return this->mpris->canSetFullscreen(); }

void MprisRootAdaptor::Raise()
{
    this->mpris->raiseMainwindow();
}

void MprisRootAdaptor::Quit()
{
    this->mpris->quitApplication();
}

// ---------------- Player adaptor ----------------

MprisPlayerAdaptor::MprisPlayerAdaptor(Mpris2 *parent)
: QDBusAbstractAdaptor(parent),
  mpris(parent)
{
    this->setAutoRelaySignals(true);
}

QString MprisPlayerAdaptor::playbackStatus() const { return this->mpris->playbackStatus(); }
QString MprisPlayerAdaptor::loopStatus() const { return this->mpris->loopStatus(); }
void MprisPlayerAdaptor::setLoopStatus(const QString &status) { this->mpris->setLoopStatus(status); }
double MprisPlayerAdaptor::rate() const { return this->mpris->rate(); }
void MprisPlayerAdaptor::setRate(double r) { this->mpris->setRate(r); }
bool MprisPlayerAdaptor::shuffle() const { return this->mpris->shuffle(); }
void MprisPlayerAdaptor::setShuffle(bool enabled) { this->mpris->setShuffle(enabled); }
QVariantMap MprisPlayerAdaptor::metadata() const { return this->mpris->metadata(); }
double MprisPlayerAdaptor::volume() const { return this->mpris->volume(); }
void MprisPlayerAdaptor::setVolume(double vol) { this->mpris->setVolume(vol); }
qlonglong MprisPlayerAdaptor::position() const { return this->mpris->position(); }
double MprisPlayerAdaptor::minimumRate() const { return this->mpris->minimumRate(); }
double MprisPlayerAdaptor::maximumRate() const { return this->mpris->maximumRate(); }
bool MprisPlayerAdaptor::canGoNext() const { return this->mpris->canGoNext(); }
bool MprisPlayerAdaptor::canGoPrevious() const { return this->mpris->canGoPrevious(); }
bool MprisPlayerAdaptor::canPlay() const { return this->mpris->canPlay(); }
bool MprisPlayerAdaptor::canPause() const { return this->mpris->canPause(); }
bool MprisPlayerAdaptor::canSeek() const { return this->mpris->canSeek(); }
bool MprisPlayerAdaptor::canControl() const { return this->mpris->canControl(); }

void MprisPlayerAdaptor::Next() { this->mpris->next(); }
void MprisPlayerAdaptor::Previous() { this->mpris->previous(); }
void MprisPlayerAdaptor::Pause() { this->mpris->pause(); }
void MprisPlayerAdaptor::PlayPause() { this->mpris->playPause(); }
void MprisPlayerAdaptor::Stop() { this->mpris->stop(); }
void MprisPlayerAdaptor::Play() { this->mpris->play(); }
void MprisPlayerAdaptor::Seek(qlonglong offset) { this->mpris->seek(offset); }
void MprisPlayerAdaptor::SetPosition(const QDBusObjectPath &trackId, qlonglong posUsec) { this->mpris->setPosition(trackId, posUsec); }
void MprisPlayerAdaptor::OpenUri(const QString &uri) { this->mpris->openUri(uri); }

// ---------------- TrackList adaptor ----------------

MprisTrackListAdaptor::MprisTrackListAdaptor(Mpris2 *parent)
: QDBusAbstractAdaptor(parent),
  mpris(parent)
{
    this->setAutoRelaySignals(true);
}

QList<QDBusObjectPath> MprisTrackListAdaptor::Tracks() const { return this->tracks(); }
QList<QDBusObjectPath> MprisTrackListAdaptor::tracks() const { return this->mpris->trackIds(); }
bool MprisTrackListAdaptor::canEditTracks() const { return true; }

QVariantList MprisTrackListAdaptor::GetTracksMetadata(const QList<QDBusObjectPath> &tracks) const
{
    return this->mpris->getTracksMetadata(tracks);
}

void MprisTrackListAdaptor::AddTrack(const QString &uri, const QDBusObjectPath &afterTrack, bool setAsCurrent)
{
    this->mpris->addTrack(uri, afterTrack, setAsCurrent);
}

void MprisTrackListAdaptor::RemoveTrack(const QDBusObjectPath &trackId)
{
    this->mpris->removeTrack(trackId);
}

void MprisTrackListAdaptor::GoTo(const QDBusObjectPath &trackId)
{
    this->mpris->goTo(trackId);
}

#endif // USE_DBUS
