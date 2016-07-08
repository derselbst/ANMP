#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "applets/analyzer/AnalyzerApplet.h"
#include "configdialog.h"

#include "Common.h"
#include "Config.h"
#include "PlaylistFactory.h"
#include "Playlist.h"
#include "Song.h"
#include <QFileDialog>
#include <QProgressDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QResizeEvent>

#include <thread>         // std::this_thread::sleep_for
#include <chrono>
#include <utility>      // std::pair
#include <cmath>

void MainWindow::callbackSeek(void* context, frame_t pos)
{
    MainWindow* ctx = static_cast<MainWindow*>(context);
    QMetaObject::invokeMethod( ctx, "slotSeek", Qt::QueuedConnection, Q_ARG(long long, pos ) );
}

void MainWindow::callbackCurrentSongChanged(void * context)
{
    MainWindow* ctx = static_cast<MainWindow*>(context);
    QMetaObject::invokeMethod( ctx, "slotCurrentSongChanged", Qt::QueuedConnection);
}

void MainWindow::slotSeek(long long pos)
{
    QSlider* playheadSlider = this->ui->seekBar;
    bool oldState = playheadSlider->blockSignals(true);
    playheadSlider->setSliderPosition(pos);
    playheadSlider->blockSignals(oldState);

    const Song* s = this->player->getCurrentSong();
    if(s==nullptr)
    {
        return;
    }

    string temp;
    {
        temp = framesToTimeStr(pos,s->Format.SampleRate);
        QString strTimePast = QString::fromStdString(temp);
        if((void*)temp.c_str() == (void*)strTimePast.constData())
            puts("asdf");

        this->ui->labelTimePast->setText(strTimePast);
    }

    {
        temp = framesToTimeStr(s->getFrames()-pos, s->Format.SampleRate);
        QString strTimeLeft = QString("-") + QString::fromStdString(temp);
        this->ui->labelTimeLeft->setText(strTimeLeft);
    }
}

void MainWindow::slotCurrentSongChanged()
{
    QSlider* playheadSlider = this->ui->seekBar;

    const Song* s = this->player->getCurrentSong();
    if(s==nullptr)
    {
        this->setWindowTitle("ANMP");

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
            this->setWindowTitle(QString::fromStdString(s->Filename) + " :: ANMP");
        }
        else
        {
            this->setWindowTitle(interpret + " - " + title  + " :: ANMP");
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
    this->player->onPlayheadChanged += make_pair(this, &MainWindow::callbackSeek);
    this->player->onCurrentSongChanged += make_pair(this, &MainWindow::callbackCurrentSongChanged);

    this->buildFileBrowser();
    this->buildPlaylistView();
    this->createShortcuts();
}

MainWindow::~MainWindow()
{
    this->player->onPlayheadChanged -= this;
    this->player->onCurrentSongChanged -= this;

    delete this->settingsView;
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

    playShortcut = new QShortcut(QKeySequence(Qt::Key_F4), this);
    connect(playShortcut, &QShortcut::activated, this, &MainWindow::tooglePlayPause);

    QShortcut *pauseFadeShortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F4), this);
    connect(pauseFadeShortcut, &QShortcut::activated, this, &MainWindow::tooglePlayPauseFade);

    QShortcut *stopFadeShortcut = new QShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F5), this);
    connect(stopFadeShortcut, &QShortcut::activated, this, &MainWindow::stopFade);

    QShortcut *stopShortcut = new QShortcut(QKeySequence(Qt::Key_F5), this);
    connect(stopShortcut, &QShortcut::activated, this, &MainWindow::stop);

    // CHANGE CURRENT SONG SHORTS
    QShortcut *nextShortcut = new QShortcut(QKeySequence(Qt::Key_MediaNext), this);
    connect(nextShortcut, &QShortcut::activated, this, &MainWindow::on_actionNext_Song_triggered);

    nextShortcut = new QShortcut(QKeySequence(Qt::Key_F8), this);
    connect(nextShortcut, &QShortcut::activated, this, &MainWindow::on_actionNext_Song_triggered);

    QShortcut *prevShortcut = new QShortcut(QKeySequence(Qt::Key_MediaPrevious), this);
    connect(prevShortcut, &QShortcut::activated, this, &MainWindow::on_actionPrevious_Song_triggered);

    prevShortcut = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(prevShortcut, &QShortcut::activated, this, &MainWindow::on_actionPrevious_Song_triggered);

    // SEEK SHORTCUTS
    QShortcut *seekForward = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(seekForward, &QShortcut::activated, this, &MainWindow::seekForward);

    seekForward = new QShortcut(QKeySequence(Qt::Key_F6), this);
    connect(seekForward, &QShortcut::activated, this, &MainWindow::seekForward);

    QShortcut *seekBackward = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(seekBackward, &QShortcut::activated, this, &MainWindow::seekBackward);

    seekBackward = new QShortcut(QKeySequence(Qt::Key_F3), this);
    connect(seekBackward, &QShortcut::activated, this, &MainWindow::seekBackward);

    QShortcut *fastSeekForward = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Right), this);
    connect(fastSeekForward, &QShortcut::activated, this, &MainWindow::fastSeekForward);

    fastSeekForward = new QShortcut(QKeySequence(Qt::Key_F7), this);
    connect(fastSeekForward, &QShortcut::activated, this, &MainWindow::fastSeekForward);

    QShortcut *fastSeekBackward = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Left), this);
    connect(fastSeekBackward, &QShortcut::activated, this, &MainWindow::fastSeekBackward);

    fastSeekBackward = new QShortcut(QKeySequence(Qt::Key_F2), this);
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

    this->ui->listView->show();
    this->ui->treeView->show();
}

void MainWindow::buildPlaylistView()
{
    this->ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->ui->tableView->setModel(playlistModel);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    int minWidthVis = 0;
    foreach(QAbstractButton *button, this->ui->controlButtonsAlwaysVisible->buttons())
    {
        minWidthVis += button->minimumWidth();
    }

    int minWidthHideSecond = minWidthVis;
    foreach(QAbstractButton *button, this->ui->controlButtonsHideSecond->buttons())
    {
        minWidthHideSecond += button->minimumWidth();
    }

    int minWidthHideFirst = minWidthHideSecond;
    foreach(QAbstractButton *button, this->ui->controlButtonsHideFirst->buttons())
    {
        minWidthHideFirst += button->minimumWidth();
    }

    int wndWidth = event->size().width() - 140;

    if(wndWidth <= minWidthHideSecond)
    {
        foreach(QAbstractButton *button, this->ui->controlButtonsHideFirst->buttons())
        {
            button->hide();
        }
        foreach(QAbstractButton *button, this->ui->controlButtonsHideSecond->buttons())
        {
            button->hide();
        }
    }
    else if(wndWidth <= minWidthHideFirst)
    {
        foreach(QAbstractButton *button, this->ui->controlButtonsHideFirst->buttons())
        {
            button->hide();
        }
        foreach(QAbstractButton *button, this->ui->controlButtonsHideSecond->buttons())
        {
            button->show();
        }

        this->ui->listView->hide();
        this->ui->treeView->hide();
    }
    else
    {
        foreach(QAbstractButton *button, this->ui->controlButtonsHideFirst->buttons())
        {
            button->show();
        }
        foreach(QAbstractButton *button, this->ui->controlButtonsHideSecond->buttons())
        {
            button->show();
        }

        this->ui->listView->show();
        this->ui->treeView->show();
    }


    QMainWindow::resizeEvent(event);
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

void MainWindow::tooglePlayPauseFade()
{
    if(this->player->getIsPlaying())
    {
        this->player->fadeout(Config::fadeTimePause);
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

void MainWindow::stopFade()
{
    this->player->fadeout(Config::fadeTimeStop);
    this->stop();
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
    const Song* s = this->player->getCurrentSong();
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
    const Song* s = this->player->getCurrentSong();
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
    this->analyzerWindow = new AnalyzerApplet(this->player, this);
    this->analyzerWindow->setAnalyzer(AnalyzerApplet::AnalyzerType::Block);
    this->analyzerWindow->startGraphics();
    this->analyzerWindow->show();
}

void MainWindow::on_actionASCII_triggered()
{
    delete this->analyzerWindow;
    this->analyzerWindow = new AnalyzerApplet(this->player, this);
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

void MainWindow::on_actionAdd_Playback_Stop_triggered()
{
    this->playlistModel->add(nullptr);
}

void MainWindow::on_actionFileBrowser_triggered(bool checked)
{
    if(checked)
    {
        this->ui->treeView->show();
        this->ui->listView->show();
    }
    else
    {
        this->ui->treeView->hide();
        this->ui->listView->hide();
    }
}

void MainWindow::on_actionSettings_triggered()
{
    if(this->settingsView == nullptr)
    {
        this->settingsView = new ConfigDialog(this);
    }
    this->settingsView->show();
}
