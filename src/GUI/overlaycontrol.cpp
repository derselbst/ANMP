#include "overlaycontrol.h"
#include "ui_overlaycontrol.h"

OverlayControl::OverlayControl(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OverlayControl)
{
    this->ui->setupUi(this);
}

OverlayControl::~OverlayControl()
{
    delete ui;
}
