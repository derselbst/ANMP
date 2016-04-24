#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "applets/analyzer/AnalyzerApplet.h"

#include "Common.h"
#include "Config.h"
#include "PlaylistFactory.h"
#include "Playlist.h"
#include "Song.h"
#include <QFileDialog>
#include <QProgressDialog>
#include <QMessageBox>
#include <QShortcut>

#include <thread>         // std::this_thread::sleep_for
#include <chrono>
#include <utility>      // std::pair
#include <cmath>

void MainWindow::onSeek(void* ctx, frame_t pos)
{
    MainWindow* context = static_cast<MainWindow*>(ctx);

    QSlider* playheadSlider = context->ui->seekBar;
    bool oldState = playheadSlider->blockSignals(true);
    playheadSlider->setSliderPosition(pos);
    playheadSlider->blockSignals(oldState);

    Song* s = context->player->getCurrentSong();
    if(s==nullptr)
    {
        return;
    }

    QString strTimePast = QString::fromStdString(framesToTimeStr(pos,s->Format.SampleRate));
    context->ui->labelTimePast->setText(strTimePast);

    QString strTimeLeft = QString("-") + QString::fromStdString(framesToTimeStr(s->getFrames()-pos, s->Format.SampleRate));
    context->ui->labelTimeLeft->setText(strTimeLeft);
}

void MainWindow::onCurrentSongChanged(void* context)
{
    MainWindow* ctx = static_cast<MainWindow*>(context);
    QSlider* playheadSlider = ctx->ui->seekBar;

        Song* s = ctx->playlistModel->current();
        if(s==nullptr)
        {
            ctx->setWindowTitle("ANMP");

            bool oldState = playheadSlider->blockSignals(true);
            playheadSlider->setSliderPosition(0);
            playheadSlider->setMaximum(0);
            playheadSlider->blockSignals(oldState);
        }
        else
        {
            QString title = QString::fromStdString(s->Metadata.Title);
            QString interpret = QString::fromStdString(s->Metadata.Artist);
            if(title == "" || interpret == "")
            {
                ctx->setWindowTitle(QString::fromStdString(s->Filename) + " :: ANMP");
            }
            else
            {
                ctx->setWindowTitle(interpret + " - " + title  + " :: ANMP");
            }
            playheadSlider->setMaximum(s->getFrames());
        }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // init UI
    this->ui->setupUi(this);

    this->setWindowState(Qt::WindowMaximized);

    // set callbacks
    this->player->onPlayheadChanged += make_pair(this, &MainWindow::onSeek);
     this->player->onCurrentSongChanged += make_pair(this, &MainWindow::onCurrentSongChanged);

    this->buildFileBrowser();
    this->buildPlaylistView();
    this->createShortcuts();
}

MainWindow::~MainWindow()
{
    this->player->onPlayheadChanged -= this;
    this->player->onCurrentSongChanged -= this;

    delete this->analyzerWindow;
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

    QShortcut *fastSeekForward = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Right), this);
    connect(fastSeekForward, &QShortcut::activated, this, &MainWindow::fastSeekForward);

    QShortcut *fastSeekBackward = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Left), this);
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

void MainWindow::closeEvent(QCloseEvent* e)
{
    qApp->quit();
}

void MainWindow::on_actionAdd_Songs_triggered()
{
  const QString dir;
  const QStringList fileNames = QFileDialog::getOpenFileNames(this, "Open Audio Files", dir, "");//Wave Files (*.wav);;Text Files (*.txt)

  QProgressDialog progress("Adding files...", "Abort", 0, fileNames.count(), this);
     progress.setWindowModality(Qt::WindowModal);
     progress.show();
      QApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
      for(int i=0; !progress.wasCanceled() && i<fileNames.count(); i++)
      {
          // only redraw progress dialog on every tenth song
          if(i%(static_cast<int>(fileNames.count()*0.1)+1)==0)
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
    this->next();
}

void MainWindow::on_actionPrevious_Song_triggered()
{
    this->previous();
}

void MainWindow::on_actionClear_Playlist_triggered()
{
    this->stop();
    this->player->setCurrentSong(nullptr);
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
    bool oldState = playbtn->blockSignals(true);
    playbtn->setChecked(true);
    playbtn->blockSignals(oldState);

}

void MainWindow::pause()
{
    this->player->pause();

    QPushButton* playbtn = this->ui->playButton;
    bool oldState = playbtn->blockSignals(true);
    playbtn->setChecked(false);
    playbtn->blockSignals(oldState);
}

void MainWindow::stop()
{
    this->player->stop();

    QPushButton* playbtn = this->ui->playButton;
    bool oldState = playbtn->blockSignals(true);
    playbtn->setChecked(false);
    playbtn->blockSignals(oldState);

    QSlider* playheadSlider = this->ui->seekBar;
    oldState = playheadSlider->blockSignals(true);
    playheadSlider->setSliderPosition(0);
    playheadSlider->blockSignals(oldState);


    QString nothing = QString::fromStdString(framesToTimeStr(0,1));

    QLabel* l = this->ui->labelTimePast;
    l->setText(nothing);

    l = this->ui->labelTimeLeft;
    Song* s = this->player->getCurrentSong();
    if(s==nullptr)
    {
        l->setText(nothing);
    }
    else
    {
        l->setText(QString::fromStdString(framesToTimeStr(s->getFrames(), s->Format.SampleRate)));
    }
}

void MainWindow::next()
{
    bool oldState = this->player->getIsPlaying();
    this->stop();
    this->player->next();
    if(oldState)
    {
      this->play();
    }
}

void MainWindow::previous()
{
    bool oldState = this->player->getIsPlaying();
    this->stop();
    this->player->previous();
    if(oldState)
    {
      this->play();
    }
}

void MainWindow::on_seekBar_sliderMoved(int position)
{
    this->player->seekTo(position);
}

void MainWindow::relativeSeek(int relpos)
{
    Song* s = this->playlistModel->current();
    if(s==nullptr)
    {
        return;
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
    this->relativeSeek(max(static_cast<frame_t>(this->ui->seekBar->maximum() * SeekNormal), Config::FramesToRender));
}

void MainWindow::seekBackward()
{
    this->relativeSeek(-1 * max(static_cast<frame_t>(this->ui->seekBar->maximum() * SeekNormal), Config::FramesToRender));
}

void MainWindow::fastSeekForward()
{
    this->relativeSeek(max(static_cast<frame_t>(this->ui->seekBar->maximum() * SeekFast), Config::FramesToRender));
}

void MainWindow::fastSeekBackward()
{
    this->relativeSeek(-1 * max(static_cast<frame_t>(this->ui->seekBar->maximum() * SeekFast), Config::FramesToRender));
}

void MainWindow::on_actionBlocky_triggered()
{
    delete this->analyzerWindow;
    this->analyzerWindow = new AnalyzerApplet(this->player, nullptr);
    this->analyzerWindow->setAnalyzer(AnalyzerApplet::AnalyzerType::Block);
    this->analyzerWindow->startGraphics();
    this->analyzerWindow->show();
}

void MainWindow::on_actionASCII_triggered()
{
    delete this->analyzerWindow;
    this->analyzerWindow = new AnalyzerApplet(this->player, nullptr);
    this->analyzerWindow->setAnalyzer(AnalyzerApplet::AnalyzerType::Ascii);
    this->analyzerWindow->startGraphics();
    this->analyzerWindow->show();
}

void MainWindow::on_forwardButton_clicked()
{
    this->seekForward();
}

void MainWindow::on_fforwardButton_clicked()
{
    this->fastSeekForward();
}

void MainWindow::on_nextButton_clicked()
{
    this->next();
}

void MainWindow::on_previousButton_clicked()
{
    this->previous();
}

void MainWindow::on_fbackwardButton_clicked()
{
    this->fastSeekBackward();
}

void MainWindow::on_backwardButton_clicked()
{
    this->seekBackward();
}
