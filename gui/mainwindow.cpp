/**
 * this file contains the gui logic of anmp's main window
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "PlayheadSlider.h"
#include "applets/analyzer/AnalyzerApplet.h"
#include "configdialog.h"

#include <anmp.hpp>

#include <QShortcut>
#include <QResizeEvent>

#include <thread>         // std::this_thread::sleep_for
#include <chrono>
#include <utility>      // std::pair
#include <cmath>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // init UI
    this->ui->setupUi(this);
    this->ui->seekBar->SetMainWindow(this);

    this->setWindowState(Qt::WindowMaximized);

    // set callbacks
    this->player->onPlayheadChanged += make_pair(this, &MainWindow::callbackSeek);
    this->player->onCurrentSongChanged += make_pair(this, &MainWindow::callbackCurrentSongChanged);
    this->player->onIsPlayingChanged += make_pair(this, &MainWindow::callbackIsPlayingChanged);

    this->buildFileBrowser();
    this->buildPlaylistView();
    this->createShortcuts();
}

MainWindow::~MainWindow()
{
    this->player->onPlayheadChanged -= this;
    this->player->onCurrentSongChanged -= this;
    this->player->onIsPlayingChanged -= this;

    delete this->settingsView;
    
#ifdef USE_VISUALIZER
    delete this->analyzerWindow;
#endif
    
    this->ui->seekBar->SetMainWindow(nullptr);
    delete this->ui;
    delete this->player;
    delete this->playlistModel;
}

void MainWindow::createShortcuts()
{
#define SHORTCUT(KEYSEQ) QShortcut(KEYSEQ, this, nullptr, nullptr, Qt::ApplicationShortcut)

    // PLAY PAUSE STOP SHORTS
    QShortcut *playShortcut = new SHORTCUT(QKeySequence(Qt::Key_MediaPlay));
    connect(playShortcut, &QShortcut::activated, this, &MainWindow::tooglePlayPause);

    playShortcut = new SHORTCUT(QKeySequence(Qt::Key_Space));
    connect(playShortcut, &QShortcut::activated, this, &MainWindow::tooglePlayPause);

    playShortcut = new SHORTCUT(QKeySequence(Qt::Key_F4));
    connect(playShortcut, &QShortcut::activated, this, &MainWindow::tooglePlayPause);

    QShortcut *pauseFadeShortcut = new SHORTCUT(QKeySequence(Qt::SHIFT + Qt::Key_F4));
    connect(pauseFadeShortcut, &QShortcut::activated, this, &MainWindow::tooglePlayPauseFade);

    QShortcut *stopFadeShortcut = new SHORTCUT(QKeySequence(Qt::SHIFT + Qt::Key_F5));
    connect(stopFadeShortcut, &QShortcut::activated, this, &MainWindow::stopFade);

    QShortcut *stopShortcut = new SHORTCUT(QKeySequence(Qt::Key_F5));
    connect(stopShortcut, &QShortcut::activated, this, &MainWindow::stop);

    // CHANGE CURRENT SONG SHORTS
    QShortcut *nextShortcut = new SHORTCUT(QKeySequence(Qt::Key_MediaNext));
    connect(nextShortcut, &QShortcut::activated, this, &MainWindow::on_actionNext_Song_triggered);

    nextShortcut = new SHORTCUT(QKeySequence(Qt::Key_F8));
    connect(nextShortcut, &QShortcut::activated, this, &MainWindow::on_actionNext_Song_triggered);

    QShortcut *prevShortcut = new SHORTCUT(QKeySequence(Qt::Key_MediaPrevious));
    connect(prevShortcut, &QShortcut::activated, this, &MainWindow::on_actionPrevious_Song_triggered);

    prevShortcut = new SHORTCUT(QKeySequence(Qt::Key_F1));
    connect(prevShortcut, &QShortcut::activated, this, &MainWindow::on_actionPrevious_Song_triggered);

    // SEEK SHORTCUTS
    QShortcut *seekForward = new SHORTCUT(QKeySequence(Qt::Key_Right));
    connect(seekForward, &QShortcut::activated, this, &MainWindow::seekForward);

    seekForward = new SHORTCUT(QKeySequence(Qt::Key_F6));
    connect(seekForward, &QShortcut::activated, this, &MainWindow::seekForward);

    QShortcut *seekBackward = new SHORTCUT(QKeySequence(Qt::Key_Left));
    connect(seekBackward, &QShortcut::activated, this, &MainWindow::seekBackward);

    seekBackward = new SHORTCUT(QKeySequence(Qt::Key_F3));
    connect(seekBackward, &QShortcut::activated, this, &MainWindow::seekBackward);

    QShortcut *fastSeekForward = new SHORTCUT(QKeySequence(Qt::ALT + Qt::Key_Right));
    connect(fastSeekForward, &QShortcut::activated, this, &MainWindow::fastSeekForward);

    fastSeekForward = new SHORTCUT(QKeySequence(Qt::Key_F7));
    connect(fastSeekForward, &QShortcut::activated, this, &MainWindow::fastSeekForward);

    QShortcut *fastSeekBackward = new SHORTCUT(QKeySequence(Qt::ALT + Qt::Key_Left));
    connect(fastSeekBackward, &QShortcut::activated, this, &MainWindow::fastSeekBackward);

    fastSeekBackward = new SHORTCUT(QKeySequence(Qt::Key_F2));
    connect(fastSeekBackward, &QShortcut::activated, this, &MainWindow::fastSeekBackward);

#undef SHORTCUT
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
    this->ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
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

void MainWindow::showAnalyzer(enum AnalyzerApplet::AnalyzerType type)
{
#ifdef USE_VISUALIZER
    delete this->analyzerWindow;
    this->analyzerWindow = new AnalyzerApplet(this->player, this);
    this->analyzerWindow->setAnalyzer(type);
    this->analyzerWindow->startGraphics();
    this->analyzerWindow->show();
#else
    this->showNoVisualizer();
#endif
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
        this->player->fadeout(gConfig.fadeTimePause);
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
}

void MainWindow::pause()
{
    this->player->pause();
}

void MainWindow::stopFade()
{
    this->player->fadeout(gConfig.fadeTimeStop);
    this->stop();
}

void MainWindow::stop()
{
    this->player->stop();

    // dont call the slot directly, a call might still be pending, making a direct call here useless
    QMetaObject::invokeMethod( this, "slotSeek", Qt::QueuedConnection, Q_ARG(long long, 0 ) );
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
    this->relativeSeek(max(static_cast<frame_t>(this->ui->seekBar->maximum() * SeekNormal), gConfig.FramesToRender));
}

void MainWindow::seekBackward()
{
    this->relativeSeek(-1 * max(static_cast<frame_t>(this->ui->seekBar->maximum() * SeekNormal), gConfig.FramesToRender));
}

void MainWindow::fastSeekForward()
{
    this->relativeSeek(max(static_cast<frame_t>(this->ui->seekBar->maximum() * SeekFast), gConfig.FramesToRender));
}

void MainWindow::fastSeekBackward()
{
    this->relativeSeek(-1 * max(static_cast<frame_t>(this->ui->seekBar->maximum() * SeekFast), gConfig.FramesToRender));
}

