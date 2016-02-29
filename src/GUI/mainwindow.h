#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "Player.h"
#include "IPlaylist.h"
#include "Playlist.h"


#include <QStringList>
#include <QStringListModel>
#include <QAbstractItemView>


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
    Player* player = nullptr;
    IPlaylist* playlist = new Playlist();




    QStringListModel *model;





private slots:
    void on_actionAdd_Songs_triggered();
    void initPlayer();
    void on_initButton_clicked();
    void on_actionPlay_triggered();
    void on_actionStop_triggered();
    void on_actionPause_triggered();
    void on_actionNext_Song_triggered();
};

#endif // MAINWINDOW_H
