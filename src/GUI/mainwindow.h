#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Player.h"
#include "IPlaylist.h"
#include "Playlist.h"
#include "PlaylistModel.h"


#include <QStringList>
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <QFileSystemModel>

class AnalyzerApplet;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static void callbackSeek(void*, frame_t pos);
    static void callbackCurrentSongChanged(void*);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* e) override;

private:
    // percentage to seek normally
    const float SeekNormal = 0.02;
    // percentage for fast seek
    const float SeekFast = 0.1;

    Ui::MainWindow *ui = nullptr;
    AnalyzerApplet * analyzerWindow = nullptr;

    PlaylistModel* playlistModel = new PlaylistModel(this);
    Player* player = new Player(this->playlistModel);

    QFileSystemModel *drivesModel = new QFileSystemModel(this);
    QFileSystemModel *filesModel = new QFileSystemModel(this);

    void buildPlaylistView();
    void buildFileBrowser();
    void createShortcuts();

    void relativeSeek(int ms);


private slots:
  friend class PlaylistModel;

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
};

#endif // MAINWINDOW_H
