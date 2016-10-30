#include "configdialog.h"
#include "ui_configdialog.h"

#include "Config.h"

#include <QFileDialog>

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

    this->ui->checkUsfHle->setChecked(Config::useHle);

    this->ui->spinGmeSampleRate->setValue(Config::gmeSampleRate);
    this->ui->spinGmeSfx->setValue(Config::gmeEchoDepth);
    this->ui->checkGmeAccurate->setChecked(Config::gmeAccurateEmulation);
    this->ui->checkGmeForever->setChecked(Config::gmePlayForever);

    this->ui->checkChorus->setChecked(Config::FluidsynthEnableChorus);

    this->ui->spinFluidSampleRate->setValue(Config::FluidsynthSampleRate);
    this->ui->checkMultiChannel->setChecked(Config::FluidsynthMultiChannel);
    this->ui->defaultSF2Path->setText(QString::fromStdString(Config::FluidsynthDefaultSoundfont));
    this->ui->checkForceDefaultSf->setChecked(Config::FluidsynthForceDefaultSoundfont);
    this->ui->checkReverb->setChecked(Config::FluidsynthEnableReverb);
    this->ui->spinRoomSize->setValue(Config::FluidsynthRoomSize);
    this->ui->spinDamping->setValue(Config::FluidsynthDamping);
    this->ui->spinWidth->setValue(Config::FluidsynthWidth);
    this->ui->spinLevel->setValue(Config::FluidsynthLevel);
    this->ui->checkChorus->setChecked(Config::FluidsynthEnableChorus);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
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

void ConfigDialog::on_browseSF2_clicked()
{
    QString sf2 = QFileDialog::getOpenFileName(this, "Select Soundfont");

    this->ui->defaultSF2Path->setText(sf2);
}

void ConfigDialog::on_buttonBox_accepted()
{
    Config::RenderWholeSong = this->ui->checkRenderWhole->isChecked();
    Config::useAudioNormalization = this->ui->checkAudioNorm->isChecked();
    Config::PreRenderTime = static_cast<unsigned int>(this->ui->spinPreRenderTime->value());

    Config::useLoopInfo = this->ui->checkLoopInfo->isChecked();
    Config::overridingGlobalLoopCount = this->ui->spinLoopCount->value();
    Config::fadeTimeStop = this->ui->spinFadeStop->value();
    Config::fadeTimePause = this->ui->spinFadePause->value();

    Config::useHle = this->ui->checkUsfHle->isChecked();

    Config::gmeEchoDepth = static_cast<float>(this->ui->spinGmeSfx->value());
    Config::gmeSampleRate = static_cast<unsigned int>(this->ui->spinGmeSampleRate->value());
    Config::gmeAccurateEmulation = this->ui->checkGmeAccurate->isChecked();
    Config::gmePlayForever = this->ui->checkGmeForever->isChecked();

    Config::FluidsynthSampleRate = static_cast<unsigned int>(this->ui->spinFluidSampleRate->value());
    Config::FluidsynthMultiChannel = this->ui->checkMultiChannel->isChecked();
    Config::FluidsynthDefaultSoundfont = this->ui->defaultSF2Path->text().toUtf8().constData();
    Config::FluidsynthForceDefaultSoundfont = this->ui->checkForceDefaultSf->isChecked();
    Config::FluidsynthEnableReverb = this->ui->checkReverb->isChecked();
    Config::FluidsynthRoomSize = this->ui->spinRoomSize->value();
    Config::FluidsynthDamping = this->ui->spinDamping->value();
    Config::FluidsynthWidth = this->ui->spinWidth->value();
    Config::FluidsynthLevel = this->ui->spinLevel->value();
    Config::FluidsynthEnableChorus = this->ui->checkChorus->isChecked();

}
