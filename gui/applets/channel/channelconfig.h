#ifndef CHANNELCONFIG_H
#define CHANNELCONFIG_H

#include <QDockWidget>

namespace Ui {
class ChannelConfig;
}
class Player;
struct SongFormat;

class ChannelConfig : public QDockWidget
{
    Q_OBJECT

public:
    explicit ChannelConfig(Player *player, QWidget *parent = 0);
    ~ChannelConfig();

private:
    Ui::ChannelConfig *ui = nullptr;
    Player* player = nullptr;
    const SongFormat* currentFormat = nullptr;

    static void callbackCurrentSongChanged(void * context);

private slots:
    void onSongChanged();
};

#endif // CHANNELCONFIG_H
