/**
 * this file provides callback functions for anmp's internal player. By that it gets enabled to communicate back to the qt gui.
 */


#include "PlaylistModel.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"


void MainWindow::callbackIsPlayingChanged(void *context, bool isPlaying, Nullable<string> msg)
{
    MainWindow *ctx = static_cast<MainWindow *>(context);
    QMetaObject::invokeMethod(ctx, "slotIsPlayingChanged", Qt::QueuedConnection, Q_ARG(bool, isPlaying), Q_ARG(bool, msg.hasValue), Q_ARG(QString, QString::fromStdString(msg.Value)));
}

void MainWindow::callbackSeek(void *context, frame_t pos)
{
    MainWindow *ctx = static_cast<MainWindow *>(context);
    QMetaObject::invokeMethod(ctx, "slotSeek", Qt::QueuedConnection, Q_ARG(long long, pos));
}

void MainWindow::callbackCurrentSongChanged(void *context, const Song *newSong)
{
    MainWindow *ctx = static_cast<MainWindow *>(context);
    QMetaObject::invokeMethod(ctx, "slotCurrentSongChanged", Qt::QueuedConnection, Q_ARG(const Song *, newSong));
    QMetaObject::invokeMethod(ctx->playlistModel, "slotCurrentSongChanged", Qt::QueuedConnection, Q_ARG(const Song *, newSong));
}
