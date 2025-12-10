#pragma once

#ifdef USE_DBUS

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QObject>
#include <QVariantMap>
#include "types.h"

class MainWindow;
class Player;
class Playlist;
class PlaylistModel;
class Song;
class MprisRootAdaptor;
class MprisPlayerAdaptor;
class MprisTrackListAdaptor;

class Mpris2 : public QObject
{
    Q_OBJECT

    public:
    static constexpr const char ServiceName[] = "org.mpris.MediaPlayer2.anmp";
    static constexpr const char ObjectPath[] = "/org/mpris/MediaPlayer2";
    static constexpr const char RootInterface[] = "org.mpris.MediaPlayer2";
    static constexpr const char PlayerInterface[] = "org.mpris.MediaPlayer2.Player";
    static constexpr const char TrackListInterface[] = "org.mpris.MediaPlayer2.TrackList";

    Mpris2(MainWindow *window, Player *player, Playlist *playlist, PlaylistModel *playlistModel, QObject *parent = nullptr);
    ~Mpris2();

    void updatePlaybackStatus(bool isPlaying);
    void updatePosition(frame_t frame);
    void updateCurrentSong(const Song *song);
    void emitSeeked(qlonglong posUsec);
    void refreshTrackList();

    QString identity() const;
    QString desktopEntry() const;
    QStringList supportedUriSchemes() const;
    QStringList supportedMimeTypes() const;
    bool canQuit() const;
    bool canRaise() const;
    bool hasTrackList() const;
    bool fullscreen() const;
    void setFullscreen(bool enabled);
    bool canSetFullscreen() const;

    QString playbackStatus() const;
    QString loopStatus() const;
    void setLoopStatus(const QString &status);
    double rate() const;
    void setRate(double r);
    bool shuffle() const;
    void setShuffle(bool enabled);
    double volume() const;
    void setVolume(double vol);
    qlonglong position() const;
    double minimumRate() const;
    double maximumRate() const;
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool canPlay() const;
    bool canPause() const;
    bool canSeek() const;
    bool canControl() const;
    QVariantMap metadata() const;
    QList<QDBusObjectPath> trackIds() const;
    QDBusObjectPath currentTrackId() const;
    int indexForTrackId(const QDBusObjectPath &trackId) const;
    QVariantMap metadataForIndex(int index) const;

    void raiseMainwindow();
    void quitApplication();

    void next();
    void previous();
    void pause();
    void playPause();
    void stop();
    void play();
    void seek(qlonglong offsetUsec);
    void setPosition(const QDBusObjectPath &trackId, qlonglong posUsec);
    void openUri(const QString &uri);

    QVariantList getTracksMetadata(const QList<QDBusObjectPath> &tracks) const;
    void addTrack(const QString &uri, const QDBusObjectPath &afterTrack, bool setAsCurrent);
    void removeTrack(const QDBusObjectPath &trackId);
    void goTo(const QDBusObjectPath &trackId);

    signals:
    void Seeked(qlonglong position);

    private slots:
    void onSongAdded(const QString &file, int cur, int total);

    private:
    void emitPropertiesChanged(const QString &iface, const QVariantMap &changed, const QStringList &invalidated = QStringList());

    MainWindow *window = nullptr;
    Player *player = nullptr;
    Playlist *playlist = nullptr;
    PlaylistModel *playlistModel = nullptr;

    MprisRootAdaptor *rootAdaptor = nullptr;
    MprisPlayerAdaptor *playerAdaptor = nullptr;
    MprisTrackListAdaptor *trackAdaptor = nullptr;

    QString playbackState = "Stopped";
    QString loopState = "None";
    bool shuffleState = false;
    double rateState = 1.0;
    double volumeState = 1.0;
    bool fullscreenState = false;
    qlonglong positionUsec = 0;
    uint32_t currentSampleRate = 0;
    QVariantMap currentMetadata;
    QString pendingSetCurrentPath;
};

class MprisRootAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", Mpris2::RootInterface)
    Q_PROPERTY(bool CanQuit READ canQuit)
    Q_PROPERTY(bool CanRaise READ canRaise)
    Q_PROPERTY(bool HasTrackList READ hasTrackList)
    Q_PROPERTY(QString Identity READ identity)
    Q_PROPERTY(QString DesktopEntry READ desktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes)
    Q_PROPERTY(bool Fullscreen READ fullscreen WRITE setFullscreen)
    Q_PROPERTY(bool CanSetFullscreen READ canSetFullscreen)

    public:
    explicit MprisRootAdaptor(Mpris2 *parent);

    bool canQuit() const;
    bool canRaise() const;
    bool hasTrackList() const;
    QString identity() const;
    QString desktopEntry() const;
    QStringList supportedUriSchemes() const;
    QStringList supportedMimeTypes() const;
    bool fullscreen() const;
    void setFullscreen(bool enabled);
    bool canSetFullscreen() const;

    public slots:
    void Raise();
    void Quit();

    private:
    Mpris2 *mpris = nullptr;
};

class MprisPlayerAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", Mpris2::PlayerInterface)
    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(QString LoopStatus READ loopStatus WRITE setLoopStatus)
    Q_PROPERTY(double Rate READ rate WRITE setRate)
    Q_PROPERTY(bool Shuffle READ shuffle WRITE setShuffle)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(qlonglong Position READ position)
    Q_PROPERTY(double MinimumRate READ minimumRate)
    Q_PROPERTY(double MaximumRate READ maximumRate)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanSeek READ canSeek)
    Q_PROPERTY(bool CanControl READ canControl)

    public:
    explicit MprisPlayerAdaptor(Mpris2 *parent);

    QString playbackStatus() const;
    QString loopStatus() const;
    void setLoopStatus(const QString &status);
    double rate() const;
    void setRate(double r);
    bool shuffle() const;
    void setShuffle(bool enabled);
    QVariantMap metadata() const;
    double volume() const;
    void setVolume(double vol);
    qlonglong position() const;
    double minimumRate() const;
    double maximumRate() const;
    bool canGoNext() const;
    bool canGoPrevious() const;
    bool canPlay() const;
    bool canPause() const;
    bool canSeek() const;
    bool canControl() const;

    public slots:
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong offset);
    void SetPosition(const QDBusObjectPath &trackId, qlonglong posUsec);
    void OpenUri(const QString &uri);

    signals:
    void Seeked(qlonglong position);

    private:
    Mpris2 *mpris = nullptr;
};

class MprisTrackListAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", Mpris2::TrackListInterface)
    Q_PROPERTY(QList<QDBusObjectPath> Tracks READ tracks)
    Q_PROPERTY(bool CanEditTracks READ canEditTracks)

    public:
    explicit MprisTrackListAdaptor(Mpris2 *parent);

    QList<QDBusObjectPath> Tracks() const;
    QList<QDBusObjectPath> tracks() const;
    bool canEditTracks() const;

    public slots:
    QVariantList GetTracksMetadata(const QList<QDBusObjectPath> &tracks) const;
    void AddTrack(const QString &uri, const QDBusObjectPath &afterTrack, bool setAsCurrent);
    void RemoveTrack(const QDBusObjectPath &trackId);
    void GoTo(const QDBusObjectPath &trackId);

    signals:
    void TrackListReplaced(const QList<QDBusObjectPath> &tracks, const QDBusObjectPath &currentTrack);
    void TrackAdded(const QVariantMap &metadata, const QDBusObjectPath &afterTrack);
    void TrackRemoved(const QDBusObjectPath &trackId);
    void TrackMetadataChanged(const QVariantMap &metadata, const QDBusObjectPath &trackId);

    private:
    Mpris2 *mpris = nullptr;
};

#endif // USE_DBUS
