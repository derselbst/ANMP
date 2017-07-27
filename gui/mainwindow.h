#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "applets/analyzer/AnalyzerApplet.h"

#include <QMainWindow>
#include <QString>

namespace Ui
{
class MainWindow;
}

class ConfigDialog;
class AnalyzerApplet;
class QFileSystemModel;
class QStandardItemModel;
class QItemSelection;
class PlaylistModel;
class Player;
struct SongFormat;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.anmp")

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static void callbackSeek(void*, frame_t pos);
    static void callbackCurrentSongChanged(void*);
    static void callbackIsPlayingChanged(void* context, bool isPlaying, Nullable<string> msg);

protected:
    void resizeEvent(QResizeEvent* event) override;
//     void closeEvent(QCloseEvent* e) override;

private:
    // percentage to seek normally
    const float SeekNormal = 0.02;
    // percentage for fast seek
    const float SeekFast = 0.1;

    Ui::MainWindow *ui = nullptr;

    QFileSystemModel *drivesModel = nullptr;
    QFileSystemModel *filesModel = nullptr;
    PlaylistModel* playlistModel = nullptr;
    Player* player = nullptr;
    QStandardItemModel* channelConfigModel = nullptr;

    AnalyzerApplet * analyzerWindow = nullptr;
    ConfigDialog* settingsView = nullptr;

    void buildPlaylistView();
    void buildChannelConfig();
    void buildFileBrowser();
    void createShortcuts();

    void relativeSeek(int ms);
    void enableSeekButtons(bool isEnabled);
    void updateChannelConfig(const SongFormat& currentFormat);
    
    void showAnalyzer(enum AnalyzerApplet::AnalyzerType type);
#ifndef USE_VISUALIZER
    void showNoVisualizer();
#endif

    void showError(const QString& msg, const QString& gen="An Error occurred");

    // must be private because qdbuscpp2xml only generates garbage for it
    void DoChannelMuting(bool (*pred)(bool voiceIsMuted, bool voiceIsSelected));

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
    void AddSongs(QStringList); // only for dbus
    void MuteSelectedVoices();
    void UnmuteSelectedVoices();
    void SoloSelectedVoices();
    void MuteAllVoices();
    void UnmuteAllVoices();
    void ToggleAllVoices();
    void ToggleSelectedVoices(const QModelIndex &index);
    
protected slots:
    friend class PlaylistModel;
    friend class PlayheadSlider;


    void slotIsPlayingChanged(bool isPlaying, bool, QString);
    void slotSeek(long long);
    void slotCurrentSongChanged();

    void shufflePlaylist();
    void clearPlaylist();

    void on_treeView_clicked(const QModelIndex &index);
    void selectSong(const QModelIndex &index);

    void reinitAudioDriver();

    void aboutQt();
    void aboutAnmp();

    void updateStatusBar(QString file, int, int);

    void on_actionFileBrowser_triggered(bool checked);

    void settingsDialogAccepted();
};

#endif // MAINWINDOW_H
