#include "configdialog.h"
#include "ui_configdialog.h"

#include "Config.h"

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    this->ui->setupUi(this);

    this->ui->spinPreRenderTime->setValue(Config::PreRenderTime);
    this->ui->checkAudioNorm->setChecked(Config::useAudioNormalization);
    this->ui->checkRenderWhole->setChecked(Config::RenderWholeSong);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::on_checkRenderWhole_clicked(bool checked)
{
    Config::RenderWholeSong = checked;
}
