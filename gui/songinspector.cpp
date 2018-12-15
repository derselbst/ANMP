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
    QWidget *scroll = this->ui->scrollAreaGeneralWidget;
    QLayout *oldLayout = scroll->layout();
    delete oldLayout;

    QFormLayout *form = new QFormLayout(scroll);
    form->addRow("File:", new MySelectableLabel(QString::fromStdString(s->Filename), scroll));
    form->addRow("File Offset:", new MySelectableLabel(QString::number(s->fileOffset.Value), scroll));
    form->addRow("File Length:", new MySelectableLabel(QString::number(::framesToMs(s->getFrames(), s->Format.SampleRate) / 1000.0) + " s", scroll));
    form->addRow("Sample Rate:", new MySelectableLabel(QString::number(s->Format.SampleRate) + " Hz", scroll));
    form->addRow("Sample Format:", new MySelectableLabel(SampleFormatName[static_cast<int>(s->Format.SampleFormat)], scroll));
    form->addRow("Audio Channels:", new MySelectableLabel(QString::number(s->Format.Channels()), scroll));
    form->addRow("Audio Voices:", new MySelectableLabel(QString::number(s->Format.Voices), scroll));
    form->addRow("Bit Rate:", new MySelectableLabel(QString::number(s->Format.getBitrate() / 1024) + " kBit/s", scroll));

    scroll->setLayout(form);
}

void SongInspector::fillMetadata(const SongInfo &m)
{
    QWidget *scroll = this->ui->scrollAreaMetaWidget;
    QLayout *oldLayout = scroll->layout();
    delete oldLayout;

    QFormLayout *form = new QFormLayout(scroll);
    form->addRow("Composer", new MySelectableLabel(QString::fromStdString(m.Composer), scroll));
    form->addRow("Artist", new MySelectableLabel(QString::fromStdString(m.Artist), scroll));
    form->addRow("Album", new MySelectableLabel(QString::fromStdString(m.Album), scroll));
    form->addRow("Title", new MySelectableLabel(QString::fromStdString(m.Title), scroll));
    form->addRow("Track", new MySelectableLabel(QString::fromStdString(m.Track), scroll));
    form->addRow("Year", new MySelectableLabel(QString::fromStdString(m.Year), scroll));
    form->addRow("Genre", new MySelectableLabel(QString::fromStdString(m.Genre), scroll));
    form->addRow("Comment", new MySelectableLabel(QString::fromStdString(m.Comment), scroll));

    scroll->setLayout(form);
}

SongInspector::~SongInspector()
{
    delete ui;
}
