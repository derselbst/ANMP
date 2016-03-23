#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "Common.h"
#include "PlaylistFactory.h"
#include "Playlist.h"
#include "Song.h"
#include <QFileDialog>
#include <QProgressDialog>
#include <QMessageBox>
#include <QShortcut>

#include <thread>         // std::this_thread::sleep_for
#include <chrono>

void MainWindow::onSeek(void* context, frame_t pos)
{
    QSlider* playheadSlider = static_cast<MainWindow*>(context)->ui->seekBar;
    bool oldState = playheadSlider->blockSignals(true);
    playheadSlider->setSliderPosition(pos);
    playheadSlider->blockSignals(oldState);
}

void MainWindow::onCurrentSongChanged(void* context)
{
    QSlider* playheadSlider = static_cast<MainWindow*>(context)->ui->seekBar;

        Song* s = static_cast<MainWindow*>(context)->playlistModel->current();
        if(s==nullptr)
        {
            // TODO set Window Title
            bool oldState = playheadSlider->blockSignals(true);
            playheadSlider->setSliderPosition(0);
            playheadSlider->setMaximum(0);
            playheadSlider->blockSignals(oldState);
        }
        else
        {
            // TODO set Window Title
            playheadSlider->setMaximum(s->getFrames());
        }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // init UI
    this->ui->setupUi(this);

    // set callbacks
    this->player->playheadChanged=MainWindow::onSeek;
    this->player->currentSongChanged=MainWindow::onCurrentSongChanged;
    this->player->callbackContext=this;

    this->buildFileBrowser();
    this->buildPlaylistView();
    this->createShortcuts();
}

MainWindow::~MainWindow()
{
    delete this->ui;
    delete this->player;
    delete this->playlistModel;
}

void MainWindow::createShortcuts()
{
    // PLAY PAUSE STOP SHORTS
    QShortcut *playShortcut = new QShortcut(QKeySequence(Qt::Key_MediaPlay), this);
    connect(playShortcut, &QShortcut::activated, this, &MainWindow::tooglePlayPause);

    playShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(playShortcut, &QShortcut::activated, this, &MainWindow::tooglePlayPause);

    // CHANGE CURRENT SONG SHORTS
    QShortcut *nextShortcut = new QShortcut(QKeySequence(Qt::Key_MediaNext), this);
    connect(nextShortcut, &QShortcut::activated, this, &MainWindow::on_actionNext_Song_triggered);

    QShortcut *prevShortcut = new QShortcut(QKeySequence(Qt::Key_MediaPrevious), this);
    connect(prevShortcut, &QShortcut::activated, this, &MainWindow::on_actionPrevious_Song_triggered);

    // SEEK SHORTCUTS
    QShortcut *seekForward = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(seekForward, &QShortcut::activated, this, &MainWindow::seekForward);

    QShortcut *seekBackward = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(seekBackward, &QShortcut::activated, this, &MainWindow::seekBackward);

    QShortcut *fastSeekForward = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Right), this);
    connect(fastSeekForward, &QShortcut::activated, this, &MainWindow::fastSeekForward);

    QShortcut *fastSeekBackward = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_Right), this);
    connect(fastSeekBackward, &QShortcut::activated, this, &MainWindow::fastSeekBackward);
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
      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
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

void MainWindow::on_playButton_toggled(bool)
{
    this->tooglePlayPause();
}

void MainWindow::on_stopButton_clicked()
{
    this->stop();
}

void MainWindow::tooglePlayPause()
{
    if(this->player->getIsPlaying())
    {
        this->pause();
    }
    else
    {
        this->play();
    }
}

void MainWindow::play()
{
    this->player->play();

    QPushButton* playbtn = this->ui->playButton;
    playbtn->setText("Pause");
    bool oldState = playbtn->blockSignals(true);
    playbtn->setChecked(true);
    playbtn->blockSignals(oldState);

}

void MainWindow::pause()
{
    this->player->pause();

    QPushButton* playbtn = this->ui->playButton;
    playbtn->setText("Play");
    bool oldState = playbtn->blockSignals(true);
    playbtn->setChecked(false);
    playbtn->blockSignals(oldState);
}

void MainWindow::stop()
{
    this->player->stop();

    QPushButton* playbtn = this->ui->playButton;
    playbtn->setText("Play");
    bool oldState = playbtn->blockSignals(true);
    playbtn->setChecked(false);
    playbtn->blockSignals(oldState);

    QSlider* playheadSlider = this->ui->seekBar;
    oldState = playheadSlider->blockSignals(true);
    playheadSlider->setSliderPosition(0);
    playheadSlider->blockSignals(oldState);
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
    this->player->seekTo(position);
}

void MainWindow::relativeSeek(int ms)
{
    Song* s = this->playlistModel->current();
    if(s==nullptr)
    {
        return;
    }

    int relpos = static_cast<int>(msToFrames(abs(ms), s->Format.SampleRate));
    if(ms<0)
    {
        relpos *= -1;
    }

    int pos = this->ui->seekBar->sliderPosition()+relpos;
    if(pos < 0)
    {
        pos=0;
    }
    else if(pos > this->ui->seekBar->maximum())
    {
        pos=this->ui->seekBar->maximum();
    }
    this->player->seekTo(pos);
}

void MainWindow::seekForward()
{
    this->relativeSeek(SeekNormal);
}

void MainWindow::seekBackward()
{
    this->relativeSeek(-SeekNormal);
}

void MainWindow::fastSeekForward()
{
    this->relativeSeek(SeekFast);
}

void MainWindow::fastSeekBackward()
{
    this->relativeSeek(-SeekFast);
}
