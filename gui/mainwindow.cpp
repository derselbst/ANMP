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
#include <QFileDialog>

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

    connect(this->ui->playButton,       &QPushButton::toggled, this, [this](bool){this->MainWindow::tooglePlayPause();});
    connect(this->ui->stopButton,       &QPushButton::clicked, this, &MainWindow::stop);

    connect(this->ui->forwardButton,    &QPushButton::clicked, this, &MainWindow::seekForward);
    connect(this->ui->backwardButton,   &QPushButton::clicked, this, &MainWindow::seekBackward);
    connect(this->ui->fforwardButton,   &QPushButton::clicked, this, &MainWindow::fastSeekForward);
    connect(this->ui->fbackwardButton,  &QPushButton::clicked, this, &MainWindow::fastSeekBackward);

    connect(this->ui->nextButton,       &QPushButton::clicked, this, &MainWindow::next);
    connect(this->ui->previousButton,   &QPushButton::clicked, this, &MainWindow::previous);


    connect(this->ui->actionPlay,       &QAction::triggered, this, &MainWindow::play);
    connect(this->ui->actionPause,      &QAction::triggered, this, &MainWindow::pause);
    connect(this->ui->actionStop,       &QAction::triggered, this, &MainWindow::stop);

    connect(this->ui->actionNext,       &QAction::triggered, this, &MainWindow::next);
    connect(this->ui->actionPrevious,   &QAction::triggered, this, &MainWindow::previous);

    connect(this->ui->actionReinitAudioDriver, &QAction::triggered, this, &MainWindow::reinitAudioDriver);


    connect(this->ui->actionAdd_Songs,        &QAction::triggered, this, [this]{this->playlistModel->asyncAdd(QFileDialog::getOpenFileNames(this, "Open Audio Files", QString(), ""));});
    connect(this->ui->actionAdd_Playback_Stop,&QAction::triggered, this, [this]{this->playlistModel->add(nullptr);});
    connect(this->ui->actionShuffle_Playst,   &QAction::triggered, this, &MainWindow::shufflePlaylist);
    connect(this->ui->actionClear_Playlist,   &QAction::triggered, this, &MainWindow::clearPlaylist);

    connect(this->ui->actionASCII,      &QAction::triggered, this, [this]{this->showAnalyzer(AnalyzerApplet::AnalyzerType::Ascii);});
    connect(this->ui->actionBlocky,     &QAction::triggered, this, [this]{this->showAnalyzer(AnalyzerApplet::AnalyzerType::Block);});

    connect(this->ui->actionSettings,   &QAction::triggered, this, [this]{this->settingsView->show();});
    connect(this->settingsView,         &ConfigDialog::accepted, this, &MainWindow::settingsDialogAccepted);


    connect(this->ui->actionAbout_Qt,   &QAction::triggered, this, &MainWindow::aboutQt);
    connect(this->ui->actionAbout_ANMP, &QAction::triggered, this, &MainWindow::aboutAnmp);


    connect(this->ui->seekBar,          &PlayheadSlider::sliderMoved, this, [this](int position){this->player->seekTo(position);});


    this->setWindowState(Qt::WindowMaximized);

    // set callbacks for the underlying player
    this->player->onPlayheadChanged    += make_pair(this, &MainWindow::callbackSeek);
    this->player->onCurrentSongChanged += make_pair(this, &MainWindow::callbackCurrentSongChanged);
    this->player->onIsPlayingChanged   += make_pair(this, &MainWindow::callbackIsPlayingChanged);

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
    this->ui->playButton->setToolTip(this->ui->playButton->toolTip() + " [F4]");

    QShortcut *pauseFadeShortcut = new SHORTCUT(QKeySequence(Qt::SHIFT + Qt::Key_F4));
    connect(pauseFadeShortcut, &QShortcut::activated, this, &MainWindow::tooglePlayPauseFade);

    QShortcut *stopFadeShortcut = new SHORTCUT(QKeySequence(Qt::SHIFT + Qt::Key_F5));
    connect(stopFadeShortcut, &QShortcut::activated, this, &MainWindow::stopFade);

    QShortcut *stopShortcut = new SHORTCUT(QKeySequence(Qt::Key_F5));
    connect(stopShortcut, &QShortcut::activated, this, &MainWindow::stop);
    this->ui->stopButton->setToolTip(this->ui->stopButton->toolTip() + " [F5]");

    // CHANGE CURRENT SONG SHORTS
    QShortcut *nextShortcut = new SHORTCUT(QKeySequence(Qt::Key_MediaNext));
    connect(nextShortcut, &QShortcut::activated, this, &MainWindow::next);

    nextShortcut = new SHORTCUT(QKeySequence(Qt::Key_F8));
    connect(nextShortcut, &QShortcut::activated, this, &MainWindow::next);
    this->ui->nextButton->setToolTip(this->ui->nextButton->toolTip() + " [F8]");

    QShortcut *prevShortcut = new SHORTCUT(QKeySequence(Qt::Key_MediaPrevious));
    connect(prevShortcut, &QShortcut::activated, this, &MainWindow::previous);

    prevShortcut = new SHORTCUT(QKeySequence(Qt::Key_F1));
    connect(prevShortcut, &QShortcut::activated, this, &MainWindow::previous);
    this->ui->previousButton->setToolTip(this->ui->previousButton->toolTip() + " [F1]");

    // SEEK SHORTCUTS
    QShortcut *seekForward = new SHORTCUT(QKeySequence(Qt::Key_Right));
    connect(seekForward, &QShortcut::activated, this, &MainWindow::seekForward);

    seekForward = new SHORTCUT(QKeySequence(Qt::Key_F6));
    connect(seekForward, &QShortcut::activated, this, &MainWindow::seekForward);
    this->ui->forwardButton->setToolTip(this->ui->forwardButton->toolTip() + " [F6]");

    QShortcut *seekBackward = new SHORTCUT(QKeySequence(Qt::Key_Left));
    connect(seekBackward, &QShortcut::activated, this, &MainWindow::seekBackward);

    seekBackward = new SHORTCUT(QKeySequence(Qt::Key_F3));
    connect(seekBackward, &QShortcut::activated, this, &MainWindow::seekBackward);
    this->ui->backwardButton->setToolTip(this->ui->backwardButton->toolTip() + " [F3]");

    QShortcut *fastSeekForward = new SHORTCUT(QKeySequence(Qt::ALT + Qt::Key_Right));
    connect(fastSeekForward, &QShortcut::activated, this, &MainWindow::fastSeekForward);

    fastSeekForward = new SHORTCUT(QKeySequence(Qt::Key_F7));
    connect(fastSeekForward, &QShortcut::activated, this, &MainWindow::fastSeekForward);
    this->ui->fforwardButton->setToolTip(this->ui->fforwardButton->toolTip() + " [F7]");

    QShortcut *fastSeekBackward = new SHORTCUT(QKeySequence(Qt::ALT + Qt::Key_Left));
    connect(fastSeekBackward, &QShortcut::activated, this, &MainWindow::fastSeekBackward);

    fastSeekBackward = new SHORTCUT(QKeySequence(Qt::Key_F2));
    connect(fastSeekBackward, &QShortcut::activated, this, &MainWindow::fastSeekBackward);
    this->ui->fbackwardButton->setToolTip(this->ui->fbackwardButton->toolTip() + " [F2]");

    QShortcut *settings = new SHORTCUT(QKeySequence(Qt::Key_F12));
    connect(settings, &QShortcut::activated, this, [this]{this->settingsView->show();});

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

#ifndef USE_VISUALIZER
void MainWindow::showNoVisualizer()
{
    QMessageBox msgBox;
    msgBox.setText("Unsupported");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setDetailedText("ANMP was built without Qt5OpenGL. No visualizers available.");
    msgBox.exec();
}
#endif

void MainWindow::tooglePlayPause()
{
    if(this->player->IsPlaying())
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
    if(this->player->IsPlaying())
    {
        this->player->fadeout(gConfig.fadeTimePause);
        this->pause();
    }
    else
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


void MainWindow::enableSeekButtons(bool isEnabled)
{
    this->ui->forwardButton->setEnabled(isEnabled);
    this->ui->fforwardButton->setEnabled(isEnabled);
    this->ui->backwardButton->setEnabled(isEnabled);
    this->ui->fbackwardButton->setEnabled(isEnabled);
}
