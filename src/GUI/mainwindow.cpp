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





    QString rootPath = qgetenv("HOME");
    drivesModel->setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
    ui->treeView->setModel(drivesModel);
    ui->treeView->setRootIndex(drivesModel->setRootPath(rootPath+"/../"));
    ui->treeView->hideColumn(1);
    ui->treeView->hideColumn(2);
    ui->treeView->hideColumn(3);

    filesModel->setFilter(QDir::NoDotAndDotDot | QDir::Files);
    ui->listView->setModel(filesModel);
    ui->listView->setRootIndex(filesModel->setRootPath(rootPath));


this->buildPlaylistView();
}

MainWindow::~MainWindow()
{
    delete this->ui;
    delete this->player;
    delete this->playlist;
}

void MainWindow::buildPlaylistView()
{
    // Create model
        model = new QStandardItemModel(0,3,this);



    model->setHeaderData(0, Qt::Horizontal, "Title");
        model->setHeaderData(1, Qt::Horizontal, "Interpret");
        model->setHeaderData(2, Qt::Horizontal, "Album");



        this->ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        this->ui->tableView->setModel(model);
}

void MainWindow::on_actionAdd_Songs_triggered()
{
  const QString dir;
  const QStringList fileNames = QFileDialog::getOpenFileNames(this, "Open WAV File", dir, "");//Wave Files (*.wav);;Text Files (*.txt)

      for(int i=0; i<fileNames.count(); i++)
      {

      Song* newSong = PlaylistFactory::addSong(*this->playlist, fileNames.at(i).toUtf8().constData());

      if(newSong==nullptr)
          continue;

          // Get the position
          int row = model->rowCount();

          // Enable add one or more rows
          model->insertRows(row,1);

          // Title
          QModelIndex index = model->index(row,0);
          this->model->setData(index,fileNames.at(i));

          // Interpret
          index = model->index(row,1);
          this->model->setData(index,QString(newSong->Metadata.Artist.c_str()));

          // Album
          index = model->index(row,2);
          this->model->setData(index,QString(newSong->Metadata.Album.c_str()));

          this->ui->tableView->setCurrentIndex(index);
      }

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

void MainWindow::on_actionPrevious_Song_triggered()
{
    this->player->stop();
    this->player->previous();
    this->player->play();
}

void MainWindow::on_actionClear_Playlist_triggered()
{                                                                                                                                                                                
      this->playlist->clear();
}

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    QString sPath = drivesModel->fileInfo(index).absoluteFilePath();
    ui->listView->setRootIndex(filesModel->setRootPath(sPath));
}


void MainWindow::on_tableView_doubleClicked(const QModelIndex &index)
{
    this->player->stop();
    Song* songToPlay = this->playlist->getSong(index.row());
    this->player->setCurrentSong(songToPlay);
    this->player->play();

}
