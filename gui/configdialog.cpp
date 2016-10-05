#include "configdialog.h"
#include "ui_configdialog.h"

#include "Config.h"

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    this->ui->setupUi(this);

    bool oldState = this->ui->comboBoxAudioDriver->blockSignals(true);
    for(int i=AudioDriver_t::BEGIN; i<AudioDriver_t::END; i++)
    {
        this->ui->comboBoxAudioDriver->insertItem(i, AudioDriverName[i]);
    }
    this->ui->comboBoxAudioDriver->setCurrentIndex(static_cast<int>(Config::audioDriver));
    this->ui->comboBoxAudioDriver->blockSignals(oldState);

    this->ui->spinPreRenderTime->setValue(Config::PreRenderTime);
    this->ui->checkAudioNorm->setChecked(Config::useAudioNormalization);
    this->ui->checkRenderWhole->setChecked(Config::RenderWholeSong);

    this->ui->checkLoopInfo->setChecked(Config::useLoopInfo);
    this->ui->spinFadePause->setValue(Config::fadeTimePause);
    this->ui->spinFadeStop->setValue(Config::fadeTimeStop);
    this->ui->spinLoopCount->setValue(Config::overridingGlobalLoopCount);

    this->ui->usfUseHle->setChecked(Config::useHle);

    this->ui->spinGmeSampleRate->setValue(Config::gmeSampleRate);
    this->ui->spinGmeSfx->setValue(Config::gmeEchoDepth);
    this->ui->checkGmeAccurate->setChecked(Config::gmeAccurateEmulation);
    this->ui->checkGmeForever->setChecked(Config::gmePlayForever);
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

void ConfigDialog::on_usfUseHle_clicked(bool checked)
{
    Config::useHle = checked;
}

void ConfigDialog::on_spinGmeSfx_valueChanged(double arg1)
{
    Config::gmeEchoDepth = static_cast<float>(arg1);
}

void ConfigDialog::on_spinGmeSampleRate_valueChanged(int arg1)
{
    Config::gmeSampleRate = arg1;
}

void ConfigDialog::on_checkGmeAccurate_clicked(bool checked)
{
    Config::gmeAccurateEmulation = checked;
}

void ConfigDialog::on_checkGmeForever_clicked(bool checked)
{
    Config::gmePlayForever = checked;
}

void ConfigDialog::on_comboBoxAudioDriver_currentIndexChanged(int index)
{
    Config::audioDriver = static_cast<AudioDriver_t>(index);
/** in case we ever use bitmasks for audioDriver

    if(Config::audioDriver & (AudioDriver_t::WAVE | AudioDriver_t::ebur128))
    {
        Config::RenderWholeSong = false;
        this->ui->checkRenderWhole->setEnabled(false);
        this->ui->checkRenderWhole->setChecked(false);

        if(Config::audioDriver & AudioDriver_t::ebur128)
        {
            Config::useAudioNormalization = false;
            this->ui->checkAudioNorm->setEnabled(false);
            this->ui->checkAudioNorm->setChecked(false);
        }
    }
    else
    {
        this->ui->checkRenderWhole->setEnabled(true);
        this->ui->checkAudioNorm->setEnabled(true);
    }*/

    switch(Config::audioDriver)
    {
#ifdef USE_EBUR128
    case AudioDriver_t::ebur128:
        Config::useAudioNormalization = false;
        this->ui->checkAudioNorm->setEnabled(false);
        this->ui->checkAudioNorm->setChecked(false);
        [[fallthrough]];
#endif
    case AudioDriver_t::WAVE:
        Config::RenderWholeSong = false;
        this->ui->checkRenderWhole->setEnabled(false);
        this->ui->checkRenderWhole->setChecked(false);
        break;

    default:
        this->ui->checkRenderWhole->setEnabled(true);
        this->ui->checkAudioNorm->setEnabled(true);
        break;
    }
}
