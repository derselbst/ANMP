#include "songinspector.h"
#include "ui_songinspector.h"
#include "ChannelConfigView.h"
#include "ChannelConfigModel.h"

#include <QFormLayout>
#include <QLabel>
#include <QStandardItemModel>
#include <QHeaderView>
#include <anmp.hpp>

SongInspector::SongInspector(const Song *s, QWidget *parent)
: QDialog(parent),
  ui(new Ui::SongInspector),
  channelConfigModel(new ChannelConfigModel(0, 1, this))
{
    this->ui->setupUi(this);
    std::string basename = ::mybasename(s->Filename);
    this->setWindowTitle(basename.c_str());
    this->FillView(s);
}

void SongInspector::FillView(const Song *s)
{
    this->fillGeneral(s);
    this->fillChannelConfig(s);
    this->fillMetadata(s->Metadata);
}

void SongInspector::fillGeneral(const Song *s)
{
    const std::vector<loop_t> loops = s->getLoopArray();
    QTableWidget *tab = this->ui->tableGeneral;
    tab->clearContents();
    tab->setRowCount(8 + loops.size()*3);
    tab->verticalHeader()->setVisible(false);
    tab->horizontalHeader()->setVisible(false);
    tab->setColumnWidth(0, 150);

    unsigned int row=0;

    QTableWidgetItem *key = new QTableWidgetItem("File");
    tab->setItem(row,0,key);
    QTableWidgetItem *value = new QTableWidgetItem(QString::fromStdString(s->Filename));
    value->setToolTip(value->text());
    tab->setItem(row,1,value);

    row++;

    key = new QTableWidgetItem("File Offset");
    tab->setItem(row,0,key);
    if(s->fileOffset.hasValue)
    {
        value = new QTableWidgetItem(QString::number(s->fileOffset.Value));
        tab->setItem(row,1,value);
    }

    row++;

    key = new QTableWidgetItem("File Length");
    tab->setItem(row,0,key);
    {
        frame_t frames = s->getFrames();
        auto sec = ::framesToMs(frames, s->Format.SampleRate) / 1000.0;
        auto min = sec / 60.0;
        value = new QTableWidgetItem(QString::number(frames) + " frames == " + QString::number(sec, 'f', 2) + " s == " + QString::number(min, 'f', 2) + " min");
        value->setToolTip(value->text());
        tab->setItem(row,1,value);
    }

    row++;

    key = new QTableWidgetItem("Sample Rate");
    tab->setItem(row,0,key);
    value = new QTableWidgetItem(QString::number(s->Format.SampleRate) + " Hz");
    tab->setItem(row,1,value);

    row++;

    key = new QTableWidgetItem("Sample Format");
    tab->setItem(row,0,key);
    value = new QTableWidgetItem(SampleFormatName[static_cast<int>(s->Format.SampleFormat)]);
    tab->setItem(row,1,value);

    row++;

    key = new QTableWidgetItem("Audio Channels (total)");
    tab->setItem(row,0,key);
    value = new QTableWidgetItem(QString::number(s->Format.Channels()));
    tab->setItem(row,1,value);

    row++;

    key = new QTableWidgetItem("Audio Voices");
    tab->setItem(row,0,key);
    value = new QTableWidgetItem(QString::number(s->Format.Voices));
    tab->setItem(row,1,value);

    row++;

    key = new QTableWidgetItem("Bit Rate");
    tab->setItem(row,0,key);
    value = new QTableWidgetItem(QString::number(s->Format.getBitrate() / 1024) + " kBit/s");
    tab->setItem(row,1,value);

    row++;

    for(unsigned int i=0; i<loops.size(); i++)
    {
        const auto& l = loops[i];

        key = new QTableWidgetItem("Loop " + QString::number(i));
        tab->setItem(row,0,key);
        value = new QTableWidgetItem("Count: " + ((l.count == 0) ? "inf" : QString::number(l.count)));
        tab->setItem(row++,1,value);

        value = new QTableWidgetItem("Start at frame " + QString::number(l.start));
        tab->setItem(row++,1,value);

        value = new QTableWidgetItem("Stop at frame " + QString::number(l.stop));
        tab->setItem(row++,1,value);
    }
}


void SongInspector::fillChannelConfig(const Song *s)
{
    this->channelConfigModel->updateChannelConfig(&s->Format);

    this->channelView = new ChannelConfigView(this->ui->scrollAreaChannel);
    channelView->setModel(this->channelConfigModel);

    this->ui->scrollAreaChannel->setWidget(channelView);
    channelView->show();
}

void SongInspector::fillMetadata(const SongInfo &m)
{
    QTableWidget *tab = this->ui->tableMeta;
    tab->clearContents();
    tab->setRowCount(8);
    static const QStringList headerStrList{"Property","Value"};
    tab->setHorizontalHeaderLabels(headerStrList);
    tab->verticalHeader()->setVisible(false);
    tab->horizontalHeader()->setVisible(true); // don't rely on the properties reported by QDesigner

    QTableWidgetItem *key = new QTableWidgetItem("Composer");
    tab->setItem(0,0,key);
    QTableWidgetItem *value = new QTableWidgetItem(QString::fromStdString(m.Composer));
    tab->setItem(0,1,value);

    key = new QTableWidgetItem("Artist");
    tab->setItem(1,0,key);
    value = new QTableWidgetItem(QString::fromStdString(m.Artist));
    tab->setItem(1,1,value);

    key = new QTableWidgetItem("Album");
    tab->setItem(2,0,key);
    value = new QTableWidgetItem(QString::fromStdString(m.Album));
    tab->setItem(2,1,value);

    key = new QTableWidgetItem("Title");
    tab->setItem(3,0,key);
    value = new QTableWidgetItem(QString::fromStdString(m.Title));
    tab->setItem(3,1,value);

    key = new QTableWidgetItem("Track");
    tab->setItem(4,0,key);
    value = new QTableWidgetItem(QString::fromStdString(m.Track));
    tab->setItem(4,1,value);

    key = new QTableWidgetItem("Year");
    tab->setItem(5,0,key);
    value = new QTableWidgetItem(QString::fromStdString(m.Year));
    tab->setItem(5,1,value);

    key = new QTableWidgetItem("Genre");
    tab->setItem(6,0,key);
    value = new QTableWidgetItem(QString::fromStdString(m.Genre));
    tab->setItem(6,1,value);

    key = new QTableWidgetItem("Comment");
    tab->setItem(7,0,key);
    value = new QTableWidgetItem(QString::fromStdString(m.Comment));
    tab->setItem(7,1,value);
}

SongInspector::~SongInspector()
{
    delete ui;
}
