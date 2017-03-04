#include "configdialog.h"
#include "ui_configdialog.h"

#include <anmp.hpp>

#include <QFileDialog>

ConfigDialog::ConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigDialog)
{
    this->ui->setupUi(this);

    bool oldState = this->ui->comboBoxAudioDriver->blockSignals(true);
    for(int i=static_cast<int>(AudioDriver_t::BEGIN); i<static_cast<int>(AudioDriver_t::END); i++)
    {
        this->ui->comboBoxAudioDriver->insertItem(i, AudioDriverName[i]);
    }
    this->ui->comboBoxAudioDriver->setCurrentIndex(static_cast<int>(gConfig.audioDriver));
    this->ui->comboBoxAudioDriver->blockSignals(oldState);

    this->ui->spinPreRenderTime->setValue(gConfig.PreRenderTime);
    this->ui->checkAudioNorm->setChecked(gConfig.useAudioNormalization);
    this->ui->checkRenderWhole->setChecked(gConfig.RenderWholeSong);

    this->ui->checkLoopInfo->setChecked(gConfig.useLoopInfo);
    this->ui->spinFadePause->setValue(gConfig.fadeTimePause);
    this->ui->spinFadeStop->setValue(gConfig.fadeTimeStop);
    this->ui->spinLoopCount->setValue(gConfig.overridingGlobalLoopCount);

    this->ui->checkUsfHle->setChecked(gConfig.useHle);

    this->ui->spinGmeSampleRate->setValue(gConfig.gmeSampleRate);
    this->ui->spinGmeSfx->setValue(gConfig.gmeEchoDepth);
    this->ui->checkGmeAccurate->setChecked(gConfig.gmeAccurateEmulation);
    this->ui->checkGmeForever->setChecked(gConfig.gmePlayForever);

    this->ui->checkChorus->setChecked(gConfig.FluidsynthEnableChorus);

    this->ui->spinFluidSampleRate->setValue(gConfig.FluidsynthSampleRate);
    this->ui->checkMultiChannel->setChecked(gConfig.FluidsynthMultiChannel);
    this->ui->defaultSF2Path->setText(QString::fromStdString(gConfig.FluidsynthDefaultSoundfont));
    this->ui->checkForceDefaultSf->setChecked(gConfig.FluidsynthForceDefaultSoundfont);
    this->ui->checkReverb->setChecked(gConfig.FluidsynthEnableReverb);
    this->ui->spinRoomSize->setValue(gConfig.FluidsynthRoomSize);
    this->ui->spinDamping->setValue(gConfig.FluidsynthDamping);
    this->ui->spinWidth->setValue(gConfig.FluidsynthWidth);
    this->ui->spinLevel->setValue(gConfig.FluidsynthLevel);
    this->ui->checkChorus->setChecked(gConfig.FluidsynthEnableChorus);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::on_comboBoxAudioDriver_currentIndexChanged(int index)
{
    gConfig.audioDriver = static_cast<AudioDriver_t>(index);
    /** in case we ever use bitmasks for audioDriver

        if(gConfig.audioDriver & (AudioDriver_t::Wave | AudioDriver_t::Ebur128))
        {
            gConfig.RenderWholeSong = false;
            this->ui->checkRenderWhole->setEnabled(false);
            this->ui->checkRenderWhole->setChecked(false);

            if(gConfig.audioDriver & AudioDriver_t::Ebur128)
            {
                gConfig.useAudioNormalization = false;
                this->ui->checkAudioNorm->setEnabled(false);
                this->ui->checkAudioNorm->setChecked(false);
            }
        }
        else
        {
            this->ui->checkRenderWhole->setEnabled(true);
            this->ui->checkAudioNorm->setEnabled(true);
        }*/

    switch(gConfig.audioDriver)
    {
#ifdef USE_EBUR128
    case AudioDriver_t::Ebur128:
        gConfig.useAudioNormalization = false;
        this->ui->checkAudioNorm->setEnabled(false);
        this->ui->checkAudioNorm->setChecked(false);
        [[fallthrough]];
#endif
    case AudioDriver_t::Wave:
        gConfig.RenderWholeSong = false;
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
    string dir = ::mydirname(gConfig.FluidsynthDefaultSoundfont);

    QString selFilter = "SoundFont (*.sf2)";
    QString sf2 = QFileDialog::getOpenFileName(this, "Select Soundfont", QString::fromStdString(dir), "SoundFont (*.sf2);;All files (*.*)", &selFilter);

    this->ui->defaultSF2Path->setText(sf2);
}

void ConfigDialog::on_buttonBox_accepted()
{
    gConfig.RenderWholeSong = this->ui->checkRenderWhole->isChecked();
    gConfig.useAudioNormalization = this->ui->checkAudioNorm->isChecked();
    gConfig.PreRenderTime = static_cast<unsigned int>(this->ui->spinPreRenderTime->value());

    gConfig.useLoopInfo = this->ui->checkLoopInfo->isChecked();
    gConfig.overridingGlobalLoopCount = this->ui->spinLoopCount->value();
    gConfig.fadeTimeStop = this->ui->spinFadeStop->value();
    gConfig.fadeTimePause = this->ui->spinFadePause->value();

    gConfig.useHle = this->ui->checkUsfHle->isChecked();

    gConfig.gmeEchoDepth = static_cast<float>(this->ui->spinGmeSfx->value());
    gConfig.gmeSampleRate = static_cast<unsigned int>(this->ui->spinGmeSampleRate->value());
    gConfig.gmeAccurateEmulation = this->ui->checkGmeAccurate->isChecked();
    gConfig.gmePlayForever = this->ui->checkGmeForever->isChecked();

    gConfig.FluidsynthSampleRate = static_cast<unsigned int>(this->ui->spinFluidSampleRate->value());
    gConfig.FluidsynthMultiChannel = this->ui->checkMultiChannel->isChecked();
    gConfig.FluidsynthDefaultSoundfont = this->ui->defaultSF2Path->text().toUtf8().constData();
    gConfig.FluidsynthForceDefaultSoundfont = this->ui->checkForceDefaultSf->isChecked();
    gConfig.FluidsynthEnableReverb = this->ui->checkReverb->isChecked();
    gConfig.FluidsynthRoomSize = this->ui->spinRoomSize->value();
    gConfig.FluidsynthDamping = this->ui->spinDamping->value();
    gConfig.FluidsynthWidth = this->ui->spinWidth->value();
    gConfig.FluidsynthLevel = this->ui->spinLevel->value();
    gConfig.FluidsynthEnableChorus = this->ui->checkChorus->isChecked();

}
