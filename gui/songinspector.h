#ifndef SONGINSPECTOR_H
#define SONGINSPECTOR_H

#include <QDialog>

class Song;
struct SongInfo;
class ChannelConfigView;
class ChannelConfigModel;
class QStandardItemModel;

namespace Ui
{
    class SongInspector;
}

class SongInspector : public QDialog
{
    Q_OBJECT

    public:
    explicit SongInspector(const Song *, QWidget *parent = 0);
    ~SongInspector();

    void FillView(const Song *s);

    private:
    Ui::SongInspector *ui;
    ChannelConfigModel *channelConfigModel = nullptr;
    ChannelConfigView *channelView = nullptr;

    void fillGeneral(const Song *s);
    void fillChannelConfig(const Song *s);
    void fillMetadata(const SongInfo &m);
};

#endif // SONGINSPECTOR_H
