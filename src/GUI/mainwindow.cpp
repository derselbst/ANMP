#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PlaylistFactory.h"
#include "Playlist.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete this->ui;
    delete this->player;
}

void MainWindow::on_actionAdd_Songs_triggered()
{
  const QString dir;
  const QStringList fileNames = QFileDialog::getOpenFileNames(this, "Open WAV File", dir, "Wave Files (*.wav);;Text Files (*.txt)");

      for(int i=0; i<fileNames.count(); i++)
      {

      PlaylistFactory::addSong(*this->playlist, fileNames.at(i).toUtf8().constData());

      }
    // reset everything, stop playback
    // load the file
}

void MainWindow::initPlayer()
{

}

void MainWindow::on_initButton_clicked()
{
    if(this->player!=nullptr)
    {
        this->player = new Player(this->playlist);
        this->player->init();
    }
ui->statusbar->showMessage("hallo", 2000);
}

void MainWindow::on_actionPlay_triggered()
{
    this->player->play();
}

void MainWindow::on_actionStop_triggered()
{
    this->player->stop();
}

void MainWindow::on_actionPause_triggered()
{
    this->player->pause();
}

void MainWindow::on_actionNext_Song_triggered()
{
    this->player->stop();
    this->player->next();
    this->player->play();
}
