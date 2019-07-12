#ifndef CHANNELCONFIGMODEL_H
#define CHANNELCONFIGMODEL_H

#include <QStandardItemModel>

struct SongFormat;

class ChannelConfigModel : public QStandardItemModel
{
    Q_OBJECT

public:
    explicit ChannelConfigModel(int rows, int columns, QObject *parent = nullptr);
    void updateChannelConfig(const SongFormat* f);
    const SongFormat* getCurrentSongFormat() const;

private:
    const SongFormat* currentFormat = nullptr;
};

#endif // CHANNELCONFIGMODEL_H
