/**
 * this file implements all necessary qt slots, enabling reacting to user inputs
 * 
 * it does not include slots, that just are marked as slots in order to be able to 
 * call them from other slots, since those are actually meant to be more like a usual
 * subrotine.
 */


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "applets/analyzer/AnalyzerApplet.h"
#include "configdialog.h"

#include <anmp.hpp>

#include <QFileDialog>
#include <QProgressDialog>
#include <QMessageBox>


void MainWindow::slotIsPlayingChanged(bool isPlaying, bool hasMsg, QString msg)
{
    QPushButton* playbtn = this->ui->playButton;
    bool oldState = playbtn->blockSignals(true);
    if(isPlaying)
    {
        playbtn->setChecked(true);
    }
    else
    {
        playbtn->setChecked(false);
    }

    playbtn->blockSignals(oldState);

    if(hasMsg)
    {
        this->slotSeek(0);
        
        QMessageBox msgBox;
        msgBox.setText("The Playback unexpectedly stopped.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setDetailedText(msg);
        msgBox.exec();
    }
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
    PlayheadSlider* playheadSlider = this->ui->seekBar;

    const Song* s = this->player->getCurrentSong();
    if(s==nullptr)
    {
        this->setWindowTitle("ANMP");

        playheadSlider->SilentReset();
    }
    else
    {
        QString title = QString::fromStdString(s->Metadata.Title);
        QString interpret = QString::fromStdString(s->Metadata.Artist);
        if(title == "" || interpret == "")
        {
            this->setWindowTitle(QString::fromStdString(::mybasename(s->Filename)) + " :: ANMP");
        }
        else
        {
            this->setWindowTitle(interpret + " - " + title  + " :: ANMP");
        }
        playheadSlider->setMaximum(s->getFrames());

        this->enableSeekButtons(this->player->IsSeekingPossible());
    }
}


void MainWindow::on_actionAdd_Songs_triggered()
{
    const QString dir;
    const QStringList fileNames = QFileDialog::getOpenFileNames(this, "Open Audio Files", dir, "");//Wave Files (*.wav);;Text Files (*.txt)

    QProgressDialog progress("Adding files...", "Abort", 0, fileNames.count(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();
    QApplication::processEvents(QEventLoop::ExcludeSocketNotifiers);
    for(int i=0; !progress.wasCanceled() && i<fileNames.count(); i++)
    {
        // only redraw progress dialog on every tenth song
        if(i%(static_cast<int>(fileNames.count()*0.1)+1)==0)
        {
            progress.setValue(i);
            QApplication::processEvents(QEventLoop::ExcludeSocketNotifiers);
        }

        PlaylistFactory::addSong(*this->playlistModel, fileNames.at(i).toUtf8().constData());
    }

    progress.setValue(fileNames.count());
}


void MainWindow::clearPlaylist()
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


void MainWindow::on_seekBar_sliderMoved(int position)
{
    this->player->seekTo(position);
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

void MainWindow::settingsDialogAccepted()
{
    if(this->settingsView->newConfig.audioDriver != gConfig.audioDriver)
    {
        this->reinitAudioDriver();
    }
}

void MainWindow::on_actionShuffle_Playst_triggered()
{
    QItemSelection indexList = this->ui->tableView->selectionModel()->selection();

    if(indexList.empty())
    {
        this->playlistModel->shuffle(0, this->playlistModel->rowCount(this->playlistModel->index(0,0)));
    }
    else
    {
        for(QItemSelection::const_iterator i=indexList.cbegin(); i!=indexList.cend(); ++i)
        {
            int top = i->top();
            int btm = i->bottom();

            QModelIndex b = this->playlistModel->index(btm,0);
            QModelIndex t = this->playlistModel->index(top,0);

            if(b.isValid() && t.isValid())
            {
                this->playlistModel->shuffle(min(top, btm), max(top, btm));
            }
        }
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
    bool oldState = this->player->IsPlaying();
    this->stop();
    this->player->next();
    if(oldState)
    {
        this->play();
    }
}

void MainWindow::previous()
{
    bool oldState = this->player->IsPlaying();
    this->stop();
    this->player->previous();
    if(oldState)
    {
        this->play();
    }
}

void MainWindow::reinitAudioDriver()
{
    this->player->initAudio();
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


void MainWindow::aboutQt()
{
    QMessageBox::aboutQt(this, "About Qt");
}

void MainWindow::aboutAnmp()
{
    // Stuff the about box text...
    QString text = "<p>\n";
    text += "<b>" ANMP_TITLE " - "  ANMP_SUBTITLE  "</b><br />\n";
    text += "<br />\n";
    text += "Version: " ANMP_VERSION "<br />\n";
    text += "Build: ";
    text += ::GetCompilerUsed();
    text += " ";
    text += ::GetCompilerVersionUsed();
    text += "<br />\n";


#define SUPPORT_MSG(NAME, SUP, COLOR) \
        text += "<font color=\"" COLOR "\">";\
        text += NAME " support " SUP;\
        text += "<br />";\
        text += "</font>";

#define SUPPORT_YES(NAME) SUPPORT_MSG(NAME, "enabled", "green")
#define SUPPORT_NO(NAME)  SUPPORT_MSG(NAME, "disabled", "red")


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

    text += "<br />\n";
    text += "<br />\n";
    text += "Website: <a href=\"" ANMP_WEBSITE "\">" ANMP_WEBSITE "</a><br />\n";
    text += "<br />\n";
    text += "<small>";
    text += "&copy;" ANMP_COPYRIGHT "<br />\n";
    text += "<br />\n";
    text += tr("This program is free software; you can redistribute it and/or modify it") + "<br />\n";
    text += tr("under the terms of the GNU General Public License version 2 or later.");
    text += "</small>";
    text += "</p>\n";

    QMessageBox::about(this, "About ANMP", text);

#undef SUPPORT_YES
#undef SUPPORT_NO
#undef SUPPORT_MSG
}
