#pragma once

#include <QTableView>

class QContextMenuEvent;
class QAction;
class QMenu;

class ChannelConfigView : public QTableView
{
    Q_OBJECT

public:
    ChannelConfigView(QWidget *parent = 0);
    void Select(Qt::CheckState state);

public slots:
    void MuteSelectedVoices();
    void UnmuteSelectedVoices();
    void SoloSelectedVoices();
    void MuteAllVoices();
    void UnmuteAllVoices();
    void ToggleAllVoices();
    void ToggleSelectedVoices();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

    void DoChannelMuting(bool (*pred)(bool voiceIsMuted, bool voiceIsSelected));

    QMenu *contextMenu = nullptr;

    QAction *actionSelect_Muted_Voices = nullptr;
    QAction *actionSelect_Unmuted_Voices = nullptr;
    QAction *actionMute_Selected_Voices = nullptr;
    QAction *actionUnmute_Selected_Voices = nullptr;
    QAction *actionMute_All_Voices = nullptr;
    QAction *actionUnmute_All_Voices = nullptr;
    QAction *actionToggle_All_Voices = nullptr;
    QAction *actionSolo_Selected_Voices = nullptr;
};
