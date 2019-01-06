#include "ChannelConfigModel.h"
#include <QStandardItem>
#include <QBrush>

#include <string>
#include <anmp.hpp>


ChannelConfigModel::ChannelConfigModel(int rows, int columns, QObject *parent)
    : QStandardItemModel(rows, columns, parent)
{
    static const QStringList strlist{"Channel Name"};
    this->setHorizontalHeaderLabels(strlist);
}

void ChannelConfigModel::updateChannelConfig(const SongFormat *currentFormat)
{
    this->removeRows(0, this->rowCount());

    this->currentFormat = currentFormat;
    if(currentFormat == nullptr)
    {
        return;
    }

    this->setRowCount(currentFormat->Voices);
    for (int i = 0; i < currentFormat->Voices; i++)
    {
        const std::string &voiceName = currentFormat->VoiceName[i];
        QStandardItem *item = new QStandardItem(QString::fromStdString(voiceName));

        QBrush b(currentFormat->VoiceIsMuted[i] ? Qt::red : Qt::green);
        item->setBackground(b);

        QBrush f(currentFormat->VoiceIsMuted[i] ? Qt::white : Qt::black);
        item->setForeground(f);

        item->setTextAlignment(Qt::AlignCenter);
        item->setCheckable(false);
        item->setCheckState(currentFormat->VoiceIsMuted[i] ? Qt::Unchecked : Qt::Checked);
        this->setItem(i, 0, item);
    }
}

const SongFormat* ChannelConfigModel::getCurrentSongFormat() const
{
    return this->currentFormat;
}
