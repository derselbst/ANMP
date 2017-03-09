#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "applets/analyzer/AnalyzerApplet.h"

#include <anmp.hpp>
#include "PlaylistModel.h"
#include "configdialog.h"

#include <QMainWindow>
#include <QStringList>
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <QFileSystemModel>

class AnalyzerApplet;

namespace Ui
{
class MainWindow;
}

class ConfigDialog;


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
    AnalyzerApplet * analyzerWindow = nullptr;
    
    ConfigDialog* settingsView = new ConfigDialog(this);

    PlaylistModel* playlistModel = new PlaylistModel(this);
    Player* player = new Player(this->playlistModel);

    QFileSystemModel *drivesModel = new QFileSystemModel(this);
    QFileSystemModel *filesModel = new QFileSystemModel(this);

    future<void> taskFading;
    future<void> taskAddSongs;

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


protected slots:
    friend class PlaylistModel;
    friend class PlayheadSlider;

    void slotIsPlayingChanged(bool isPlaying, bool, QString);
    void slotSeek(long long);
    void slotCurrentSongChanged();

    void on_actionAdd_Songs_triggered();
    void clearPlaylist();

    void on_treeView_clicked(const QModelIndex &index);
    void on_tableView_doubleClicked(const QModelIndex &index);
    void on_tableView_activated(const QModelIndex &index);
    void on_seekBar_sliderMoved(int position);


   //void addFiles(const QStringList&);
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

    void on_actionAdd_Playback_Stop_triggered();
    void on_actionFileBrowser_triggered(bool checked);

    void settingsDialogAccepted();
    void on_actionShuffle_Playst_triggered();
};

#endif // MAINWINDOW_H
