/**
 * this file implements all necessary qt slots, enabling reacting to user inputs
 * 
 * it does not include slots, that just are marked as slots in order to be able to 
 * call them from other slots, since those are actually meant to be more like a usual
 * subrotine.
 */


#include "ChannelConfigModel.h"
#include "PlaylistModel.h"
#include "configdialog.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_playcontrol.h"

#include <anmp.hpp>

#include <QFileSystemModel>
#include <QListView>
#include <QMessageBox>
#include <QResizeEvent>
#include <QStandardItemModel>
#include <QTreeView>

void MainWindow::slotIsPlayingChanged(bool isPlaying, bool hasMsg, const QString& msg)
{
    QPushButton *playbtn = this->playctrl->playButton;
    bool oldState = playbtn->blockSignals(true);
    if (isPlaying)
    {
        playbtn->setChecked(true);
    }
    else
    {
        playbtn->setChecked(false);
    }

    playbtn->blockSignals(oldState);

    if (hasMsg)
    {
        this->slotSeek(0);

        this->showError(msg, "The Playback unexpectedly stop");
    }
}

void MainWindow::slotBufferHealthChanged(long long framesRendered)
{
    this->playctrl->seekBar->setBufferHealth(framesRendered);
}

void MainWindow::slotSeek(long long pos)
{
    PlayheadSlider *playheadSlider = this->playctrl->seekBar;
    bool oldState = playheadSlider->blockSignals(true);
    playheadSlider->setSliderPosition(pos);
    playheadSlider->blockSignals(oldState);

    std::string temp;
    
    const Song *s = this->player->getCurrentSong();
    if (s == nullptr)
    {
        temp = framesToTimeStr(0, 1);
        QString strTime = QString::fromStdString(temp);
        this->playctrl->labelTimePast->setText(strTime);
        this->playctrl->labelTimeLeft->setText(QString("-") + strTime);
        
        return;
    }

    {
        temp = framesToTimeStr(pos, s->Format.SampleRate);
        QString strTimePast = QString::fromStdString(temp);
        this->playctrl->labelTimePast->setText(strTimePast);
    }

    {
        temp = framesToTimeStr(s->getFrames() - pos, s->Format.SampleRate);
        QString strTimeLeft = QString("-") + QString::fromStdString(temp);
        this->playctrl->labelTimeLeft->setText(strTimeLeft);
    }
}

void MainWindow::slotCurrentSongChanged(const Song* s)
{
    // fetch the current song by asking the player directly
    // rather than retrieving it as event parameter, as the song might have been already deleted by the user
    // now, being in the main thread, the user cannot delete it
    s = this->player->getCurrentSong();
    if (s == nullptr || !s->Format.IsValid())
    {
        this->setWindowTitleCustom("");
        this->channelConfigModel->updateChannelConfig(nullptr);
    }
    else
    {
        QString title = QString::fromStdString(s->Metadata.Title);
        QString interpret = QString::fromStdString(s->Metadata.Artist);
        if (title == "" || interpret == "")
        {
            this->setWindowTitleCustom(QString::fromStdString(::mybasename(s->Filename)));
        }
        else
        {
            this->setWindowTitleCustom(interpret + " - " + title);
        }
        
        this->enableSeekButtons(this->player->IsSeekingPossible());
        this->channelConfigModel->updateChannelConfig(&s->Format);
    }
    
    this->playctrl->seekBar->SlotCurrentSongChanged(s);
    this->playlistModel->SlotCurrentSongChanged(s);
}

void MainWindow::treeViewClicked(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }
    QString sPath = drivesModel->fileInfo(index).absoluteFilePath();
    this->listView->setRootIndex(filesModel->setRootPath(sPath));
}


// if a selected song gets double clicked activated (press enter) in playlist view, play it back
void MainWindow::selectSong(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }

    try
    {
        this->Stop();
        this->player->setCurrentSong(this->playlist->setCurrentSong(index.row()));
        this->Play();
    }
    catch (const std::exception &e)
    {
        this->showError(e.what());
    }
}

void MainWindow::settingsDialogAccepted()
{
    if (this->settingsView->newConfig.audioDriver != gConfig.audioDriver)
    {
        this->reinitAudioDriver();
    }
}

void MainWindow::shufflePlaylist()
{
    //     QItemSelection indexList = this->ui->tableView->selectionModel()->selection();
    //
    //     if(indexList.empty())
    //     {
    this->playlistModel->shuffle(0, this->playlistModel->rowCount(this->playlistModel->index(0, 0)));
    //     }
    //     else
    //     {
    //         for(QItemSelection::const_iterator i=indexList.cbegin(); i!=indexList.cend(); ++i)
    //         {
    //             int top = i->top();
    //             int btm = i->bottom();
    //
    //             QModelIndex b = this->playlistModel->index(btm,0);
    //             QModelIndex t = this->playlistModel->index(top,0);
    //
    //             if(b.isValid() && t.isValid())
    //             {
    //                 this->playlistModel->shuffle(min(top, btm), max(top, btm));
    //             }
    //         }
    //     }
}

void MainWindow::clearPlaylist()
{
    try
    {
        this->Stop();
        this->player->setCurrentSong(nullptr);
        this->playlistModel->clear();
    }
    catch (const std::exception &e)
    {
        this->showError(e.what(), "Clearing the Playlist failed");
    }
}

void MainWindow::TogglePlayPause()
{
    if (this->player->IsPlaying())
    {
        this->Pause();
    }
    else
    {
        this->Play();
    }
}

void MainWindow::TogglePlayPauseFade()
{
    if (this->player->IsPlaying())
    {
        this->player->fadeout(gConfig.fadeTimePause);
        this->Pause();
    }
    else
    {
        this->Play();
    }
}

void MainWindow::Play()
{
    try
    {
        this->player->play();
    }
    catch (const std::exception &e)
    {
        this->showError(e.what(), "Failed to start playback");
    }
}

void MainWindow::Pause()
{
    try
    {
        this->player->pause();
    }
    catch (const std::exception &e)
    {
        this->showError(e.what(), "Failed to pause playback");
    }
}

void MainWindow::StopFade()
{
    this->player->fadeout(gConfig.fadeTimeStop);
    this->Stop();
}

void MainWindow::Stop()
{
    try
    {
        this->player->stop();
    }
    catch (const std::exception &e)
    {
        this->showError(e.what(), "Failed to stop playback");
    }
}

void MainWindow::Next()
{
    try
    {
        bool oldState = this->player->IsPlaying();
        this->Stop();
        this->player->setCurrentSong(this->playlist->next());
        if (oldState)
        {
            this->Play();
        }
    }
    catch (const std::exception &e)
    {
        this->showError(e.what(), "Failed to play the next Song");
    }
}

void MainWindow::Previous()
{
    try
    {
        bool oldState = this->player->IsPlaying();
        this->Stop();
        this->player->setCurrentSong(this->playlist->previous());
        if (oldState)
        {
            this->Play();
        }
    }
    catch (const std::exception &e)
    {
        this->showError(e.what(), "Failed to play the previous Song");
    }
}

void MainWindow::reinitAudioDriver()
{
    try
    {
        this->player->initAudio();
    }
    catch (const std::exception &e)
    {
        this->showError(e.what(), "Unable to initialize the audio driver");
    }
}

void MainWindow::SeekForward()
{
    this->relativeSeek(std::max(static_cast<frame_t>(this->playctrl->seekBar->maximum() * this->SeekNormal), gConfig.FramesToRender));
}

void MainWindow::SeekBackward()
{
    this->relativeSeek(-1 * std::max(static_cast<frame_t>(this->playctrl->seekBar->maximum() * this->SeekNormal), gConfig.FramesToRender));
}

void MainWindow::FastSeekForward()
{
    this->relativeSeek(std::max(static_cast<frame_t>(this->playctrl->seekBar->maximum() * this->SeekFast), gConfig.FramesToRender));
}

void MainWindow::FastSeekBackward()
{
    this->relativeSeek(-1 * std::max(static_cast<frame_t>(this->playctrl->seekBar->maximum() * this->SeekFast), gConfig.FramesToRender));
}

void MainWindow::AddSongs(const QStringList& files)
{
    this->playlistModel->asyncAdd(files);
}

void MainWindow::slotSongAdded(const QString& file, int cur, int total)
{
    QStatusBar* stat = this->ui->statusbar;
    
    // avoid showMessage() updating itself every time it's message changes
    // instead, implicitly schedule an update (when re-enabling updates below) to give the main thread a chance to skip paint events
    stat->setUpdatesEnabled(false);
    
    QString text = "Adding file (";
    text += QString::number(cur);
    text += " / ";
    text += QString::number(total);
    text += ") ";
    text += file;

    if (cur == total)
    {
        stat->showMessage(text, 3000);
        this->ui->playlistView->resizeColumnsToContents();
    }
    else
    {
        stat->showMessage(text);
    }
    
    stat->setUpdatesEnabled(true);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    this->ui->dockDir->setMinimumHeight(event->size().height() / 3);

    this->QMainWindow::resizeEvent(event);
}

void MainWindow::aboutQt()
{
    QMessageBox::aboutQt(this, "About Qt");
}

void MainWindow::aboutAnmp()
{
    // build up the huge constexpr about anmp std::string
    static constexpr char text[] = "<p>\n"
                                   "<b>" ANMP_TITLE " - " ANMP_SUBTITLE "</b><br />\n"
                                   "<br />\n"
                                   "Version: " ANMP_VERSION "<br />\n"
                                   "Build: " ANMP_COMPILER_USED
                                   " " ANMP_COMPILER_VERSION_USED
                                   "<br />\n"
                                   "<br />\n"
                                   "InputLibrary support status:"
                                   "<br />\n"

#define SUPPORT_MSG(NAME, SUP, COLOR)                 \
    "<font color=\"" COLOR "\">" NAME " support " SUP \
    "<br />"                                          \
    "</font>"

#define SUPPORT_YES(NAME) SUPPORT_MSG(NAME, "enabled", "green")
#define SUPPORT_NO(NAME) SUPPORT_MSG(NAME, "disabled", "red")

#ifdef USE_LIBSND
    SUPPORT_YES("libsndfile")
#else
    SUPPORT_NO("libsndfile")
#endif

#ifdef USE_LAZYUSF
    SUPPORT_YES("lazyusf")
#else
    SUPPORT_NO("lazyusf")
#endif

#ifdef USE_AOPSF
    SUPPORT_YES("aopsf")
#else
    SUPPORT_NO("aopsf")
#endif

#ifdef USE_LIBMAD
    SUPPORT_YES("MAD")
#else
    SUPPORT_NO("MAD")
#endif

#ifdef USE_LIBGME
    SUPPORT_YES("Game-Music-Emu")
#else
    SUPPORT_NO("Game-Music-Emu")
#endif

#ifdef USE_MODPLUG
    SUPPORT_YES("ModPlug")
#else
    SUPPORT_NO("ModPlug")
#endif

#ifdef USE_OPENMPT
    SUPPORT_YES("OpenMPT")
#else
    SUPPORT_NO("OpenMPT")
#endif

#ifdef USE_VGMSTREAM
    SUPPORT_YES("VGMStream")
#else
    SUPPORT_NO("VGMStream")
#endif

#ifdef USE_FFMPEG
    SUPPORT_YES("FFMpeg")
#else
    SUPPORT_NO("FFMpeg")
#endif

#ifdef USE_FLUIDSYNTH
    SUPPORT_YES("Fluidsynth")
#else
    SUPPORT_NO("Fluidsynth")
#endif

    "<br />\n"
    "AudioDriver support status:"
    "<br />\n"

#ifdef USE_ALSA
    SUPPORT_YES("ALSA")
#else
    SUPPORT_NO("ALSA")
#endif

#ifdef USE_JACK
    SUPPORT_YES("Jack")
#else
    SUPPORT_NO("Jack")
#endif

#ifdef USE_PORTAUDIO
    SUPPORT_YES("PortAudio")
#else
    SUPPORT_NO("PortAudio")
#endif

#ifdef USE_EBUR128
    SUPPORT_YES("Audio Normalization")
#else
    SUPPORT_NO("Audio Normalization")
#endif

    "<br />\n"
    "Miscellaneous status:"
    "<br />\n"

#ifdef USE_CUE
    SUPPORT_YES("Cue Sheet")
#else
    SUPPORT_NO("Cue Sheet")
#endif

#ifdef USE_GUI
    SUPPORT_YES("Qt5 Gui")
#else
    SUPPORT_NO("Qt5 Gui")
#endif

#ifdef USE_VISUALIZER
    SUPPORT_YES("Audio Visualizer")
#else
    SUPPORT_NO("Audio Visualizer")
#endif

    "<br />\n"
    "<br />\n"
    "Website: <a href=\"" ANMP_WEBSITE "\">" ANMP_WEBSITE "</a><br />\n"
    "<br />\n"
    "<small>"
    "&copy;" ANMP_COPYRIGHT "<br />\n"
    "<br />\n"
    "This program is free software; you can redistribute it and/or modify it"
    "<br />\n"
    "under the terms of the GNU General Public License version 2."
    "</small>"
    "</p>\n";

    QMessageBox::about(this, "About ANMP", text);

#undef SUPPORT_YES
#undef SUPPORT_NO
#undef SUPPORT_MSG
}
