#include "AnalyzerApplet.h"
#include "ui_AnalyzerApplet.h"

#include "ASCIIAnalyzer.h"
#include "BlockAnalyzer.h"
#include "SpectrogramAnalyzer.h"

#include "Player.h"

#include <utility> // std::pair

AnalyzerApplet::AnalyzerApplet(Player *player, QWidget *parent)
: QMainWindow(parent),
  ui(new Ui::AnalyzerApplet)
{
    ui->setupUi(this);
    this->player = player;
}

AnalyzerApplet::~AnalyzerApplet()
{
    this->stopGraphics();

    delete this->analyzerWidget;
    delete this->ui;
}

void AnalyzerApplet::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    this->newGeometry();
}

void AnalyzerApplet::closeEvent(QCloseEvent *e)
{
    QMainWindow::closeEvent(e);

    this->stopGraphics();
}

void AnalyzerApplet::startGraphics()
{
    this->player->onPlayheadChanged += std::make_pair(this, &AnalyzerApplet::redraw);
    this->analyzerWidget->connectSignals();
}

void AnalyzerApplet::stopGraphics()
{
    this->player->onPlayheadChanged -= this;

    if (this->analyzerWidget != nullptr)
    {
        this->analyzerWidget->disconnectSignals();
    }
}

void AnalyzerApplet::newGeometry()
{
    //     if(this->analyzerWidget==nullptr)
    return;

    // Use the applet's geometry for showing the analyzer widget at the same position
    QRect analyzerGeometry = geometry();

    // Adjust widget geometry to keep the applet border intact
    //     analyzerGeometry.adjust( +3, +3, -3, -3 );

    this->analyzerWidget->setGeometry(analyzerGeometry);
}

void AnalyzerApplet::setAnalyzer(AnalyzerType type)
{
    if (this->analyzerWidget != nullptr)
    {
        throw std::logic_error("Analyzer: widget already set!");
    }

    switch (type)
    {
        default:
            this->analyzerWidget = nullptr;
            [[fallthrough]];
        case Block:
            this->analyzerWidget = new BlockAnalyzer(this);
            this->setWindowTitle("Block Analyzer");
            break;
        case Spectrogram:
            this->analyzerWidget = new SpectrogramAnalyzer(this);
            this->setWindowTitle("Spectrogram Analyzer");
            break;
        case Ascii:
            this->analyzerWidget = new ASCIIAnalyzer(this);
            this->setWindowTitle("ASCII Analyzer");
            break;
    }

    this->ui->horizontalLayout->addWidget(this->analyzerWidget);

    this->newGeometry();
    this->analyzerWidget->show();
}

void AnalyzerApplet::redraw(void *ctx, frame_t pos)
{
    AnalyzerApplet *context = static_cast<AnalyzerApplet *>(ctx);

    context->analyzerWidget->processData(context->player->getCurrentSong(), pos);
}
