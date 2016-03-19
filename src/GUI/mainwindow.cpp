#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PlaylistFactory.h"
#include "Playlist.h"
#include "Song.h"
#include <QFileDialog>
#include <QProgressDialog>
#include <QMessageBox>

#include <thread>         // std::this_thread::sleep_for
#include <chrono>

#include <iostream>
void MainWindow::onSeek(void* context, frame_t pos)
{
    static_cast<MainWindow*>(context)->ui->seekBar->setSliderPosition(pos);
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->ui->setupUi(this);

    this->player->playheadChanged=MainWindow::onSeek;
    this->player->callbackContext=this;

    this->buildFileBrowser();
    this->buildPlaylistView();
}

MainWindow::~MainWindow()
{
    delete this->ui;
    delete this->player;
    delete this->playlistModel;
}

void MainWindow::buildFileBrowser()
{
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
}

void MainWindow::buildPlaylistView()
{
    this->ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        this->ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
      this->ui->tableView->setModel(playlistModel); 
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
          // only redraw progress dialog on every tenth song
          if(i%10==0)
          {
                 progress.setValue(i);
                 QApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
          }

      PlaylistFactory::addSong(*this->playlistModel, fileNames.at(i).toUtf8().constData());
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
      this->playlistModel->clear();
}

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    if(!index.isValid())
  {
    return;
  }
    QString sPath = drivesModel->fileInfo(index).absoluteFilePath();
    ui->listView->setRootIndex(filesModel->setRootPath(sPath));
}


void MainWindow::on_tableView_doubleClicked(const QModelIndex &index)
{
    if(!index.isValid())
  {
    return;
  }
    this->stop();
    Song* songToPlay = this->playlistModel->setCurrentSong(index.row());
    this->player->setCurrentSong(songToPlay);
    this->play();

}

void MainWindow::on_tableView_activated(const QModelIndex &index)
{
  if(!index.isValid())
  {
    return;
  }
    this->stop();
    Song* songToPlay = this->playlistModel->setCurrentSong(index.row());
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
    Song* s = this->playlistModel->current();
    if(s==nullptr) return;

    this->ui->seekBar->setMaximum(s->getFrames());
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

void MainWindow::on_tableView_remove(const QModelIndexList& elements)
{
  int lastRow=0;
for(QModelIndexList::const_iterator i=elements.cbegin(); i!=elements.cend(); ++i)
{
  if(i->isValid())
  {
    this->playlistModel->remove(i->row()-lastRow);
    lastRow=i->row();
} 
}
}

void MainWindow::on_seekBar_sliderMoved(int position)
{
    cout << "seek " << position << endl;
    this->player->seekTo(position);
}
