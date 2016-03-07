#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PlaylistFactory.h"
#include "Playlist.h"
#include <QFileDialog>
#include <QProgressDialog>

#include <thread>         // std::this_thread::sleep_for
#include <chrono>

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

    this->ui->listView->hide();
    this->ui->treeView->hide();

    this->ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);


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

  QProgressDialog progress("Adding files...", "Abort", 0, fileNames.count(), this);
     progress.setWindowModality(Qt::WindowModal);
     progress.show();

      for(int i=0; !progress.wasCanceled() && i<fileNames.count(); i++)
      {
          // only redraw progress dialog on every fifth song
          if(i%10==0)
          {
                 progress.setValue(i);
                 QApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
}

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

      progress.setValue(fileNames.count());
}


void MainWindow::on_actionPlay_triggered()
{
    this->play();
}

void MainWindow::on_actionStop_triggered()
{
    this->stop();
}

void MainWindow::on_actionPause_triggered()
{
    this->pause();
}

void MainWindow::on_actionNext_Song_triggered()
{
    this->stop();
    this->player->next();
    this->play();
}

void MainWindow::on_actionPrevious_Song_triggered()
{
    this->stop();
    this->player->previous();
    this->play();
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
    this->stop();
    Song* songToPlay = this->playlist->getSong(index.row());
    this->player->setCurrentSong(songToPlay);
    this->play();

}

void MainWindow::on_playButton_toggled(bool checked)
{
    if(checked)
    {
        this->play();
    }
    else
    {
        this->pause();
    }
}

void MainWindow::on_stopButton_clicked()
{
    this->stop();
}

void MainWindow::play()
{
    this->player->play();
    this->ui->playButton->setText("Pause");
    this->ui->playButton->setChecked(true);
}

void MainWindow::pause()
{
    this->player->pause();
    this->ui->playButton->setText("Play");
    this->ui->playButton->setChecked(false);
}

void MainWindow::stop()
{
    this->player->stop();
    this->ui->playButton->setText("Play");
    this->ui->playButton->setChecked(false);
}
