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
            this->setWindowTitle(QString::fromStdString(::mybasename(s->Filename)) + " :: ANMP");
        }
        else
        {
            this->setWindowTitle(interpret + " - " + title  + " :: ANMP");
        }
        playheadSlider->setMaximum(s->getFrames());
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

void MainWindow::on_seekBar_sliderMoved(int position)
{
    this->player->seekTo(position);
}

void MainWindow::on_actionBlocky_triggered()
{
    this->showAnalyzer(AnalyzerApplet::AnalyzerType::Block);
}

void MainWindow::on_actionASCII_triggered()
{
    this->showAnalyzer(AnalyzerApplet::AnalyzerType::Ascii);
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

    AudioDriver_t oldDriver = gConfig.audioDriver;

    int ret = this->settingsView->exec();

    if(ret == QDialog::Accepted)
    {
        if(oldDriver != gConfig.audioDriver)
        {
            this->player->initAudio();
        }
        gConfig.Save();
    }

    delete this->settingsView;
    this->settingsView = nullptr;
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

void MainWindow::on_actionReinit_AudioDriver_triggered()
{
    this->player->initAudio();
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
