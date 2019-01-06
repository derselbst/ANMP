#include "songinspector.h"
#include "ui_songinspector.h"

#include <QFormLayout>
#include <QLabel>
#include <anmp.hpp>

class MySelectableLabel : public QLabel
{
public:
    MySelectableLabel(const QString& text, QWidget *parent=nullptr) : QLabel(text, parent)
    {
        this->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }
};

SongInspector::SongInspector(const Song *s, QWidget *parent)
: QDialog(parent),
  ui(new Ui::SongInspector)
{
    ui->setupUi(this);
    this->FillView(s);
}

void SongInspector::FillView(const Song *s)
{
    this->fillGeneral(s);
    this->fillMetadata(s->Metadata);
}

void SongInspector::fillGeneral(const Song *s)
{
    QTableWidget *tab = this->ui->tableGeneral;
    tab->clearContents();
    tab->setRowCount(8);
    tab->verticalHeader()->setVisible(false);
    tab->horizontalHeader()->setVisible(false);
    tab->setColumnWidth(0, 150);

    QTableWidgetItem *key = new QTableWidgetItem("File");
    tab->setItem(0,0,key);
    QTableWidgetItem *value = new QTableWidgetItem(QString::fromStdString(s->Filename));
    tab->setItem(0,1,value);

    key = new QTableWidgetItem("File Offset");
    tab->setItem(1,0,key);
    if(s->fileOffset.hasValue)
    {
        value = new QTableWidgetItem(QString::number(s->fileOffset.Value));
        tab->setItem(1,1,value);
    }

    key = new QTableWidgetItem("File Length");
    tab->setItem(2,0,key);
    value = new QTableWidgetItem(QString::number(::framesToMs(s->getFrames(), s->Format.SampleRate) / 1000.0) + " s");
    tab->setItem(2,1,value);

    key = new QTableWidgetItem("Sample Rate");
    tab->setItem(3,0,key);
    value = new QTableWidgetItem(QString::number(s->Format.SampleRate) + " Hz");
    tab->setItem(3,1,value);

    key = new QTableWidgetItem("Sample Format");
    tab->setItem(4,0,key);
    value = new QTableWidgetItem(SampleFormatName[static_cast<int>(s->Format.SampleFormat)]);
    tab->setItem(4,1,value);

    key = new QTableWidgetItem("Audio Channels (total)");
    tab->setItem(5,0,key);
    value = new QTableWidgetItem(QString::number(s->Format.Channels()));
    tab->setItem(5,1,value);

    key = new QTableWidgetItem("Audio Voices");
    tab->setItem(6,0,key);
    value = new QTableWidgetItem(QString::number(s->Format.Voices));
    tab->setItem(6,1,value);

    key = new QTableWidgetItem("Bit Rate");
    tab->setItem(7,0,key);
    value = new QTableWidgetItem(QString::number(s->Format.getBitrate() / 1024) + " kBit/s");
    tab->setItem(7,1,value);
}


void SongInspector::fillChannelConfig(const Song *s)
{
    this->ui->scrollAreaChannel->setWidget();
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
