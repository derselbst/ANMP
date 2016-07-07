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

    this->ui->checkLoopInfo->setChecked(Config::useLoopInfo);
    this->ui->spinFadePause->setValue(Config::fadeTimePause);
    this->ui->spinFadeStop->setValue(Config::fadeTimeStop);
    this->ui->spinLoopCount->setValue(Config::overridingGlobalLoopCount);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::on_checkRenderWhole_clicked(bool checked)
{
    Config::RenderWholeSong = checked;
}

void ConfigDialog::on_checkAudioNorm_clicked(bool checked)
{
    Config::useAudioNormalization = checked;
}

void ConfigDialog::on_spinPreRenderTime_valueChanged(int arg1)
{
    Config::PreRenderTime = static_cast<unsigned int>(arg1);
}

void ConfigDialog::on_checkLoopInfo_clicked(bool checked)
{
    Config::useLoopInfo = checked;
}

void ConfigDialog::on_spinLoopCount_valueChanged(int arg1)
{
    Config::overridingGlobalLoopCount = arg1;
}

void ConfigDialog::on_spinFadeStop_valueChanged(int arg1)
{
    Config::fadeTimeStop = arg1;
}

void ConfigDialog::on_spinFadePause_valueChanged(int arg1)
{
    Config::fadeTimePause = arg1;
}
