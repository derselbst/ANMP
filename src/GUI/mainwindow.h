#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Player.h"
#include "IPlaylist.h"
#include "Playlist.h"


#include <QStringList>
#include <QStringListModel>
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
    IPlaylist* playlist = new Playlist();
    Player* player = this->player = new Player(this->playlist);




    QStringListModel *model;


    QFileSystemModel        *drivesModel = new QFileSystemModel(this);
    QFileSystemModel        *filesModel = new QFileSystemModel(this);


private slots:
    void on_actionAdd_Songs_triggered();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();
    void on_actionPause_triggered();
    void on_actionNext_Song_triggered();
    void on_actionClear_Playlist_triggered();
    void on_treeView_clicked(const QModelIndex &index);
};

#endif // MAINWINDOW_H
