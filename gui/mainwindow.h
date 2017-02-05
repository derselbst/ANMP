#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "applets/analyzer/AnalyzerApplet.h"

#include <anmp.hpp>
#include "PlaylistModel.h"

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
#ifdef USE_VISUALIZER
    AnalyzerApplet * analyzerWindow = nullptr;
#endif
    
    ConfigDialog * settingsView = nullptr;

    PlaylistModel* playlistModel = new PlaylistModel(this);
    Player* player = new Player(this->playlistModel);

    QFileSystemModel *drivesModel = new QFileSystemModel(this);
    QFileSystemModel *filesModel = new QFileSystemModel(this);

    void buildPlaylistView();
    void buildFileBrowser();
    void createShortcuts();

    void relativeSeek(int ms);
    
#ifndef USE_VISUALIZER
    void showNoVisualizer();
#endif


private slots:
    friend class PlaylistModel;
    friend class PlayheadSlider;

    void slotIsPlayingChanged(bool isPlaying, bool, QString);
    void slotSeek(long long);
    void slotCurrentSongChanged();

    void on_actionAdd_Songs_triggered();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();
    void on_actionPause_triggered();
    void on_actionNext_Song_triggered();
    void on_actionClear_Playlist_triggered();
    void on_treeView_clicked(const QModelIndex &index);
    void on_tableView_doubleClicked(const QModelIndex &index);
    void on_actionPrevious_Song_triggered();
    void on_playButton_toggled(bool);
    void on_stopButton_clicked();
    void on_tableView_activated(const QModelIndex &index);
    void on_seekBar_sliderMoved(int position);

    void seekForward();
    void seekBackward();
    void fastSeekForward();
    void fastSeekBackward();

    void play();
    void pause();
    void tooglePlayPause();
    void stop();
    void tooglePlayPauseFade();
    void stopFade();
    void previous();
    void next();
    void showAnalyzer(enum AnalyzerApplet::AnalyzerType type);

    void on_actionBlocky_triggered();
    void on_actionASCII_triggered();
    void on_forwardButton_clicked();
    void on_fforwardButton_clicked();
    void on_nextButton_clicked();
    void on_previousButton_clicked();
    void on_fbackwardButton_clicked();
    void on_backwardButton_clicked();
    void on_actionAdd_Playback_Stop_triggered();
    void on_actionFileBrowser_triggered(bool checked);
    void on_actionSettings_triggered();
    void on_actionShuffle_Playst_triggered();
    void on_actionReinit_AudioDriver_triggered();
};

#endif // MAINWINDOW_H
