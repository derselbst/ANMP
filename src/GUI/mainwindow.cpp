#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PlaylistFactory.h"
#include "Playlist.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->ui->setupUi(this);






    // Create model
        model = new QStringListModel(this);

        // Make data
        QStringList List;
        List << "test" << "Reverie" << "Prelude";

        // Populate our model
        model->setStringList(List);

        // Glue model and view together
        ui->listView->setModel(model);

        // Add additional feature so that
        // we can manually modify the data in ListView
        // It may be triggered by hitting any key or double-click etc.
        ui->listView->setEditTriggers(QAbstractItemView::AnyKeyPressed | QAbstractItemView::DoubleClicked);
}

MainWindow::~MainWindow()
{
    delete this->ui;
    delete this->player;
    delete this->playlist;
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









          // Adding at the end

      for(int i=0; i<fileNames.count(); i++)
      {

          // Get the position
          int row = model->rowCount();

          // Enable add one or more rows
          model->insertRows(row,1);

          // Get the row for Edit mode
          QModelIndex index = model->index(row);

          this->model->setData(index,fileNames.at(i));
      }
}

void MainWindow::initPlayer()
{

}

void MainWindow::on_initButton_clicked()
{

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