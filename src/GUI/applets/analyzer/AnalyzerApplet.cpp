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
    this->analyzerWidget = new BlockAnalyzer(this);
    this->ui->horizontalLayout->addWidget(this->analyzerWidget);

    this->player = player;
    this->startGraphics();
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

void AnalyzerApplet::setCurrentAnalyzer( enum AnalyzerType& type )
{
//     if( m_analyzerName == name )
//         return;

    delete this->analyzerWidget;

    switch(type)
    {
    default:
        /* fall through */
    case Block:
        this->analyzerWidget = new BlockAnalyzer(this);
        break;
    }

    this->newGeometry();
    this->analyzerWidget->show();
}

void AnalyzerApplet::redraw(void* ctx, frame_t pos)
{  
  AnalyzerApplet* context = static_cast<AnalyzerApplet*>(ctx);

  context->analyzerWidget->processData(context->player->getCurrentSong(), pos);
}