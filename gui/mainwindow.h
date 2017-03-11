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
class PlaylistModel;
class Player;

class MainWindow : public QMainWindow
{
    Q_OBJECT

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

    AnalyzerApplet * analyzerWindow = nullptr;
    ConfigDialog* settingsView = nullptr;

    void buildPlaylistView();
    void buildFileBrowser();
    void createShortcuts();

    void relativeSeek(int ms);
    void enableSeekButtons(bool isEnabled);
    
#ifdef USE_VISUALIZER
    void showAnalyzer(enum AnalyzerApplet::AnalyzerType type);
#else
    void showNoVisualizer();
#endif

    void showError(const QString& msg, const QString& gen="An Error occurred");


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

    void play();
    void pause();
    void tooglePlayPause();
    void tooglePlayPauseFade();
    void stop();
    void stopFade();
    void previous();
    void next();
    void seekForward();
    void seekBackward();
    void fastSeekForward();
    void fastSeekBackward();
    void reinitAudioDriver();

    void aboutQt();
    void aboutAnmp();

    void updateStatusBar(QString file, int, int);

    void on_actionFileBrowser_triggered(bool checked);

    void settingsDialogAccepted();
};

#endif // MAINWINDOW_H
