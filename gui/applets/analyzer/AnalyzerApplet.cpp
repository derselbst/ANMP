#include "AnalyzerApplet.h"
#include "ui_AnalyzerApplet.h"

#include "BlockAnalyzer.h"
#include "ASCIIAnalyzer.h"

#include "Player.h"

#include <utility>      // std::pair

AnalyzerApplet::AnalyzerApplet(Player* player, QWidget *parent) :
    QMainWindow(parent),
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

void AnalyzerApplet::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    this->newGeometry();
}

void AnalyzerApplet::closeEvent(QCloseEvent* e)
{
    QMainWindow::closeEvent(e);

    this->stopGraphics();
}

void AnalyzerApplet::startGraphics()
{
    this->player->onPlayheadChanged += make_pair(this, &AnalyzerApplet::redraw);
    this->analyzerWidget->connectSignals();
}

void AnalyzerApplet::stopGraphics()
{
    this->player->onPlayheadChanged -= this;
    this->analyzerWidget->disconnectSignals();
}

void AnalyzerApplet::newGeometry()
{
//     if(this->analyzerWidget==nullptr)
    return;

    // Use the applet's geometry for showing the analyzer widget at the same position
    QRect analyzerGeometry = geometry();

    // Adjust widget geometry to keep the applet border intact
//     analyzerGeometry.adjust( +3, +3, -3, -3 );

    this->analyzerWidget->setGeometry( analyzerGeometry );
}

void AnalyzerApplet::setAnalyzer(AnalyzerType type )
{
//     if( m_analyzerName == name )
//         return;

    if(this->analyzerWidget!=nullptr)
    {
        this->ui->horizontalLayout->removeWidget(this->analyzerWidget);
        delete this->analyzerWidget;
    }

    switch(type)
    {
    default:
    /* fall through */
    case Block:
        this->analyzerWidget = new BlockAnalyzer(this);
        this->setWindowTitle("Block Analyzer");
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

void AnalyzerApplet::redraw(void* ctx, frame_t pos)
{
    AnalyzerApplet* context = static_cast<AnalyzerApplet*>(ctx);

    context->analyzerWidget->processData(context->player->getCurrentSong(), pos);
}
