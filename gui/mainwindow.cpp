/**
 * this file contains the gui logic of anmp's main window
 */

#include "mainwindow.h"
#include "ChannelConfigView.h"
#include "ChannelConfigModel.h"
#include "PlayheadSlider.h"
#include "PlaylistModel.h"
#include "applets/analyzer/AnalyzerApplet.h"
#include "configdialog.h"
#include "mainwindow_adaptor.h"
#include "ui_mainwindow.h"
#include "ui_playcontrol.h"

#include <anmp.hpp>

#include <QFileDialog>
#include <QFileSystemModel>
#include <QListView>
#include <QMessageBox>
#include <QShortcut>
#include <QStandardItemModel>
#include <QTableView>
#include <QTreeView>
#include <QFontDatabase>

#include <chrono>
#include <cmath>
#include <thread> // std::this_thread::sleep_for
#include <utility> // std::pair


MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent),
  ui(new Ui::MainWindow),
  playctrl(new Ui::PlayControl),
  drivesModel(new QFileSystemModel(this)),
  filesModel(new QFileSystemModel(this)),
  playlist(new Playlist()),
  playlistModel(new PlaylistModel(playlist, this)),
  player(new Player(this->playlist)),
  channelConfigModel(new ChannelConfigModel(0, 1, this)),
#ifdef USE_VISUALIZER
  analyzerWindow(new AnalyzerApplet(this->player, this)),
#endif
  settingsView(new ConfigDialog(this))
{
    qRegisterMetaType<const Song *>("const Song*");
    /*    
    // add our D-Bus interface and connect to D-Bus
    new AnmpAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/ANMP", this);

    org::anmp *iface;
    iface = new org::anmp(QString("org.anmp"), QString(), QDBusConnection::sessionBus(), this);
    //connect(iface, SIGNAL(message(QString,QString)), this, SLOT(messageSlot(QString,QString)));
    QDBusConnection::sessionBus().connect(QString("org.anmp"), QString(), "org.anmp", "TooglePlayPause", this, SLOT(tooglePlayPause()));
//     connect(iface, &org::anmp::TooglePlayPause, this, &MainWindow::TogglePlayPause);*/

    new MainWindowAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/MainWindow", this);
    dbus.registerService("org.anmp");

    // init UI
    this->ui->setupUi(this);

    QWidget *dockwid = new QWidget(this->ui->dockControl);
    this->playctrl->setupUi(dockwid);
    this->playctrl->labelTimeLeft->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    this->playctrl->labelTimePast->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    this->ui->dockControl->setWidget(dockwid);


    this->buildFileBrowser();
    this->buildPlaylistView();
    this->buildChannelConfig();

    this->ui->menuDockWindows->addAction(this->ui->dockControl->toggleViewAction());
    this->ui->menuDockWindows->addAction(this->ui->dockChannel->toggleViewAction());
    this->ui->menuDockWindows->addAction(this->ui->dockDir->toggleViewAction());
    this->ui->menuDockWindows->addAction(this->ui->dockFile->toggleViewAction());

    // connect main buttons
    connect(this->playctrl->playButton, &QPushButton::toggled, this, [this](bool) { this->MainWindow::TogglePlayPause(); });
    connect(this->playctrl->stopButton, &QPushButton::clicked, this, &MainWindow::Stop);

    connect(this->playctrl->forwardButton, &QPushButton::clicked, this, &MainWindow::SeekForward);
    connect(this->playctrl->backwardButton, &QPushButton::clicked, this, &MainWindow::SeekBackward);
    connect(this->playctrl->fforwardButton, &QPushButton::clicked, this, &MainWindow::FastSeekForward);
    connect(this->playctrl->fbackwardButton, &QPushButton::clicked, this, &MainWindow::FastSeekBackward);

    connect(this->playctrl->nextButton, &QPushButton::clicked, this, &MainWindow::Next);
    connect(this->playctrl->previousButton, &QPushButton::clicked, this, &MainWindow::Previous);


    // connect menu actions
    connect(this->ui->actionPlay, &QAction::triggered, this, &MainWindow::Play);
    connect(this->ui->actionPause, &QAction::triggered, this, &MainWindow::Pause);
    connect(this->ui->actionStop, &QAction::triggered, this, &MainWindow::Stop);

    connect(this->ui->actionNext, &QAction::triggered, this, &MainWindow::Next);
    connect(this->ui->actionPrevious, &QAction::triggered, this, &MainWindow::Previous);

    connect(this->ui->actionReinitAudioDriver, &QAction::triggered, this, &MainWindow::reinitAudioDriver);


    connect(this->ui->actionAdd_Songs, &QAction::triggered, this, [this] { this->playlistModel->asyncAdd(QFileDialog::getOpenFileNames(this, "Open Audio Files", QString(), "")); });
    connect(this->ui->actionAdd_Playback_Stop, &QAction::triggered, this,
            [this] {
                long idxTarget = this->playlist->getCurrentSongId();
                long idxAt = this->playlist->add(nullptr);
                this->playlist->move(idxAt, 0, idxTarget - idxAt + 1);
                this->playlistModel->insertRows(idxTarget, 1);
            });
    connect(this->ui->actionAdd_Playback_Stop_At_End, &QAction::triggered, this,
            [this] {
                this->playlistModel->insertRows(this->playlist->add(nullptr), 1);
            });
    connect(this->ui->actionShuffle_Playst, &QAction::triggered, this, &MainWindow::shufflePlaylist);
    connect(this->ui->actionClear_Playlist, &QAction::triggered, this, &MainWindow::clearPlaylist);

    connect(this->ui->actionASCII, &QAction::triggered, this, [this] { this->showAnalyzer(AnalyzerApplet::AnalyzerType::Ascii); });
    connect(this->ui->actionBlocky, &QAction::triggered, this, [this] { this->showAnalyzer(AnalyzerApplet::AnalyzerType::Block); });
    connect(this->ui->actionSpectrogram, &QAction::triggered, this, [this] { this->showAnalyzer(AnalyzerApplet::AnalyzerType::Spectrogram); });

    connect(this->ui->actionSettings, &QAction::triggered, this, [this] { this->settingsView->show(); });
    connect(this->settingsView, &ConfigDialog::accepted, this, &MainWindow::settingsDialogAccepted);

    connect(this->ui->actionClose, &QAction::triggered, QCoreApplication::instance(), &QCoreApplication::quit);

    connect(this->ui->actionAbout_Qt, &QAction::triggered, this, &MainWindow::aboutQt);
    connect(this->ui->actionAbout_ANMP, &QAction::triggered, this, &MainWindow::aboutAnmp);


    connect(this->playctrl->seekBar, &PlayheadSlider::sliderMoved, this, [this](int position) { this->player->seekTo(position); });


    // emitted double-clicking or pressing Enter
    connect(this->ui->playlistView, &PlaylistView::activated, this, &MainWindow::selectSong);

    connect(this->playlistModel, &PlaylistModel::SongAdded, this, &MainWindow::slotSongAdded, Qt::QueuedConnection);
    connect(this->playlistModel, &PlaylistModel::UnloadCurrentSong, this, [this] {this->player->stop(); this->player->setCurrentSong(nullptr); });

    connect(this->treeView, &QTreeView::clicked, this, &MainWindow::treeViewClicked);


    // set callbacks for the underlying player
    this->player->onPlayheadChanged += make_pair(this, &MainWindow::callbackSeek);
    this->player->onCurrentSongChanged += make_pair(this, &MainWindow::callbackCurrentSongChanged);
    this->player->onIsPlayingChanged += make_pair(this, &MainWindow::callbackIsPlayingChanged);

    this->createShortcuts();

    this->setWindowTitleCustom("");
    this->setWindowState(Qt::WindowMaximized);
}

MainWindow::~MainWindow()
{
    this->player->onPlayheadChanged -= this;
    this->player->onCurrentSongChanged -= this;
    this->player->onIsPlayingChanged -= this;

    delete this->ui;

    delete this->playctrl;

    // manually delete applets before deleting player, since they hold this.player
    delete this->analyzerWindow;

    delete this->player;
    delete this->playlistModel;
    delete this->playlist;
}

void MainWindow::createShortcuts()
{
#define SHORTCUT(KEYSEQ) QShortcut(KEYSEQ, this, nullptr, nullptr, Qt::ApplicationShortcut)

    // PLAY PAUSE STOP SHORTS
    QShortcut *playShortcut = new SHORTCUT(QKeySequence(Qt::Key_MediaPlay));
    connect(playShortcut, &QShortcut::activated, this, &MainWindow::TogglePlayPause);

    playShortcut = new SHORTCUT(QKeySequence(Qt::Key_Space));
    connect(playShortcut, &QShortcut::activated, this, &MainWindow::TogglePlayPause);

    playShortcut = new SHORTCUT(QKeySequence(Qt::Key_F4));
    connect(playShortcut, &QShortcut::activated, this, &MainWindow::TogglePlayPause);
    this->playctrl->playButton->setToolTip(this->playctrl->playButton->toolTip() + " [F4]");

    QShortcut *pauseFadeShortcut = new SHORTCUT(QKeySequence(Qt::SHIFT + Qt::Key_F4));
    connect(pauseFadeShortcut, &QShortcut::activated, this, &MainWindow::TogglePlayPauseFade);

    QShortcut *stopFadeShortcut = new SHORTCUT(QKeySequence(Qt::SHIFT + Qt::Key_F5));
    connect(stopFadeShortcut, &QShortcut::activated, this, &MainWindow::StopFade);

    QShortcut *stopShortcut = new SHORTCUT(QKeySequence(Qt::Key_F5));
    connect(stopShortcut, &QShortcut::activated, this, &MainWindow::Stop);
    this->playctrl->stopButton->setToolTip(this->playctrl->stopButton->toolTip() + " [F5]");

    // CHANGE CURRENT SONG SHORTS
    QShortcut *nextShortcut = new SHORTCUT(QKeySequence(Qt::Key_MediaNext));
    connect(nextShortcut, &QShortcut::activated, this, &MainWindow::Next);

    nextShortcut = new SHORTCUT(QKeySequence(Qt::Key_F8));
    connect(nextShortcut, &QShortcut::activated, this, &MainWindow::Next);
    this->playctrl->nextButton->setToolTip(this->playctrl->nextButton->toolTip() + " [F8]");

    QShortcut *prevShortcut = new SHORTCUT(QKeySequence(Qt::Key_MediaPrevious));
    connect(prevShortcut, &QShortcut::activated, this, &MainWindow::Previous);

    prevShortcut = new SHORTCUT(QKeySequence(Qt::Key_F1));
    connect(prevShortcut, &QShortcut::activated, this, &MainWindow::Previous);
    this->playctrl->previousButton->setToolTip(this->playctrl->previousButton->toolTip() + " [F1]");

    // SEEK SHORTCUTS
    QShortcut *seekForward = new SHORTCUT(QKeySequence(Qt::Key_Right));
    connect(seekForward, &QShortcut::activated, this, &MainWindow::SeekForward);

    seekForward = new SHORTCUT(QKeySequence(Qt::Key_F6));
    connect(seekForward, &QShortcut::activated, this, &MainWindow::SeekForward);
    this->playctrl->forwardButton->setToolTip(this->playctrl->forwardButton->toolTip() + " [F6]");

    QShortcut *seekBackward = new SHORTCUT(QKeySequence(Qt::Key_Left));
    connect(seekBackward, &QShortcut::activated, this, &MainWindow::SeekBackward);

    seekBackward = new SHORTCUT(QKeySequence(Qt::Key_F3));
    connect(seekBackward, &QShortcut::activated, this, &MainWindow::SeekBackward);
    this->playctrl->backwardButton->setToolTip(this->playctrl->backwardButton->toolTip() + " [F3]");

    QShortcut *fastSeekForward = new SHORTCUT(QKeySequence(Qt::ALT + Qt::Key_Right));
    connect(fastSeekForward, &QShortcut::activated, this, &MainWindow::FastSeekForward);

    fastSeekForward = new SHORTCUT(QKeySequence(Qt::Key_F7));
    connect(fastSeekForward, &QShortcut::activated, this, &MainWindow::FastSeekForward);
    this->playctrl->fforwardButton->setToolTip(this->playctrl->fforwardButton->toolTip() + " [F7]");

    QShortcut *fastSeekBackward = new SHORTCUT(QKeySequence(Qt::ALT + Qt::Key_Left));
    connect(fastSeekBackward, &QShortcut::activated, this, &MainWindow::FastSeekBackward);

    fastSeekBackward = new SHORTCUT(QKeySequence(Qt::Key_F2));
    connect(fastSeekBackward, &QShortcut::activated, this, &MainWindow::FastSeekBackward);
    this->playctrl->fbackwardButton->setToolTip(this->playctrl->fbackwardButton->toolTip() + " [F2]");

    QShortcut *settings = new SHORTCUT(QKeySequence(Qt::Key_F12));
    connect(settings, &QShortcut::activated, this, [this] { this->settingsView->show(); });

#undef SHORTCUT
}

void MainWindow::buildFileBrowser()
{
    QString rootPath = qgetenv("HOME");
    this->drivesModel->setFilter(QDir::NoDotAndDotDot | QDir::Dirs);

    QTreeView *treeView = this->treeView = new QTreeView(this->ui->dockDir);
    treeView->setModel(this->drivesModel);
    treeView->setRootIndex(this->drivesModel->setRootPath(rootPath + "/../"));
    treeView->hideColumn(1);
    treeView->hideColumn(2);
    treeView->hideColumn(3);
    treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    treeView->setDragEnabled(true);
    treeView->setDragDropMode(QAbstractItemView::DragOnly);
    this->ui->dockDir->setWidget(treeView);

    this->filesModel->setFilter(QDir::NoDotAndDotDot | QDir::Files);
    QListView *listView = this->listView = new QListView(this->ui->dockFile);
    listView->setModel(this->filesModel);
    listView->setRootIndex(this->filesModel->setRootPath(rootPath));
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listView->setDragEnabled(true);
    listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    this->ui->dockFile->setWidget(listView);

    listView->show();
    treeView->show();
}

void MainWindow::buildChannelConfig()
{
    this->channelView = new ChannelConfigView(this->ui->dockChannel);
    channelView->setModel(this->channelConfigModel);

    this->ui->dockChannel->setWidget(channelView);
    channelView->show();
}

void MainWindow::buildPlaylistView()
{
    this->ui->playlistView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    this->ui->playlistView->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->ui->playlistView->setModel(this->playlistModel);
}


void MainWindow::setWindowTitleCustom(QString title)
{
    if (!title.isEmpty())
    {
        title.append(" :: ");
    }
    
    title.append("ANMP " ANMP_VERSION);

    this->setWindowTitle(title);
}

void MainWindow::showAnalyzer(enum AnalyzerApplet::AnalyzerType type)
{
#ifdef USE_VISUALIZER
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

void MainWindow::relativeSeek(int relpos)
{
    const Song *s = this->player->getCurrentSong();
    if (s == nullptr)
    {
        return;
    }

    int pos = this->playctrl->seekBar->sliderPosition() + relpos;
    if (pos < 0)
    {
        pos = 0;
    }
    else if (pos > this->playctrl->seekBar->maximum())
    {
        pos = this->playctrl->seekBar->maximum();
    }
    this->player->seekTo(pos);
}


void MainWindow::enableSeekButtons(bool isEnabled)
{
    this->playctrl->seekBar->setEnabled(isEnabled);
    this->playctrl->stopButton->setEnabled(isEnabled);
    this->playctrl->forwardButton->setEnabled(isEnabled);
    this->playctrl->fforwardButton->setEnabled(isEnabled);
    this->playctrl->backwardButton->setEnabled(isEnabled);
    this->playctrl->fbackwardButton->setEnabled(isEnabled);
}

void MainWindow::showError(const QString &detail, const QString &general)
{
    QMessageBox msgBox;
    msgBox.setText(general);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setDetailedText(detail);
    msgBox.exec();
}
