#include "AnalyzerApplet.h"
#include "ui_AnalyzerApplet.h"

#include "BlockAnalyzer.h"

AnalyzerApplet::AnalyzerApplet(Player* player, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AnalyzerApplet)
{
    ui->setupUi(this);
    this->analyzerWidget = new BlockAnalyzer(this);
    this->ui->horizontalLayout->addWidget(this->analyzerWidget);

    // TODO: register callbacks
    this->player = player;
}

AnalyzerApplet::~AnalyzerApplet()
{
    delete this->analyzerWidget;
    delete this->ui;
}

void AnalyzerApplet::resizeEvent(QResizeEvent* event)
{
  QMainWindow::resizeEvent(event);

  this->newGeometry();
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
