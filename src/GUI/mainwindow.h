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


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    
    PlaylistModel* playlistModel = new PlaylistModel(this);
    Player* player = new Player(this->playlistModel);


    QFileSystemModel        *drivesModel = new QFileSystemModel(this);
    QFileSystemModel        *filesModel = new QFileSystemModel(this);

    void buildPlaylistView();
    void buildFileBrowser();
    
    void play();
    void pause();
    void stop();
    static void onSeek(void*, frame_t);


private slots:
    void on_actionAdd_Songs_triggered();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();
    void on_actionPause_triggered();
    void on_actionNext_Song_triggered();
    void on_actionClear_Playlist_triggered();
    void on_treeView_clicked(const QModelIndex &index);
    void on_tableView_doubleClicked(const QModelIndex &index);
    void on_actionPrevious_Song_triggered();
    void on_playButton_toggled(bool checked);
    void on_stopButton_clicked();
    void on_tableView_activated(const QModelIndex &index);
    void on_tableView_remove(const QModelIndexList &);
    void on_seekBar_sliderMoved(int position);
};

#endif // MAINWINDOW_H
