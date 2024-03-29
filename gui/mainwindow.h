#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "applets/analyzer/AnalyzerApplet.h"

#include <QMainWindow>
#include <QString>
#include <memory>
#include <vector>

namespace Ui
{
    class MainWindow;
    class PlayControl;
}

class ChannelConfigView;
class ChannelConfigModel;
class ConfigDialog;
class AnalyzerApplet;
class QFileSystemModel;
class QStandardItemModel;
class QItemSelection;
class QListView;
class QTreeView;
class PlaylistModel;
class Playlist;
class Player;
class Song;
struct SongFormat;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.anmp")

    public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static void callbackBufferHealth(void *, frame_t pos);
    static void callbackSeek(void *, frame_t pos);
    static void callbackCurrentSongChanged(void *, const Song *newSong);
    static void callbackIsPlayingChanged(void *context, bool isPlaying, Nullable<std::string> msg);

    protected:
    void resizeEvent(QResizeEvent *event) override;
    //     void closeEvent(QCloseEvent* e) override;

    private:
    // percentage to seek normally
    const float SeekNormal = 0.02f;
    // percentage for fast seek
    const float SeekFast = 0.1f;

    Ui::MainWindow *ui = nullptr;
    Ui::PlayControl *playctrl = nullptr;

    QFileSystemModel *drivesModel = nullptr;
    QFileSystemModel *filesModel = nullptr;
    Playlist *playlist = nullptr;
    PlaylistModel *playlistModel = nullptr;
    Player *player = nullptr;
    ChannelConfigModel *channelConfigModel = nullptr;

#ifdef USE_VISUALIZER
    std::vector<std::unique_ptr<AnalyzerApplet>> analyzerWindows;
#endif
    ConfigDialog *settingsView = nullptr;
    ChannelConfigView *channelView = nullptr;
    QListView *listView = nullptr;
    QTreeView *treeView = nullptr;

    void buildPlaylistView();
    void buildChannelConfig();
    void buildFileBrowser();
    void createShortcuts();

    void relativeSeek(int relpos);
    void enableSeekButtons(bool isEnabled);

    void showAnalyzer(enum AnalyzerApplet::AnalyzerType type);
#ifndef USE_VISUALIZER
    void showNoVisualizer();
#endif

    void showError(const QString &detail, const QString &general = "An Error occurred");

    // must be private because qdbuscpp2xml only generates garbage for it
    void DoChannelMuting(bool (*pred)(bool voiceIsMuted, bool voiceIsSelected));

    void setWindowTitleCustom(QString);

    public slots:
    void Play();
    void Pause();
    void TogglePlayPause();
    void TogglePlayPauseFade();
    void Stop();
    void StopFade();
    void Previous();
    void Next();
    void SeekForward();
    void SeekBackward();
    void FastSeekForward();
    void FastSeekBackward();
    void AddSongs(const QStringList&); // only for dbus

    protected slots:
    friend class PlaylistModel;
    friend class PlayheadSlider;


    void slotIsPlayingChanged(bool isPlaying, bool, const QString&);
    void slotSeek(long long);
    void slotBufferHealthChanged(long long);
    void slotCurrentSongChanged(const Song *s);

    void shufflePlaylist();
    void clearPlaylist();

    void treeViewClicked(const QModelIndex &index);
    void selectSong(const QModelIndex &index);

    void reinitAudioDriver();

    void aboutQt();
    void aboutAnmp();

    void slotSongAdded(const QString& file, int, int);

    void settingsDialogAccepted();
};

#endif // MAINWINDOW_H
