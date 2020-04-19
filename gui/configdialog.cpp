#include "configdialog.h"
#include "ui_configdialog.h"

#include <anmp.hpp>

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QShowEvent>

ConfigDialog::ConfigDialog(QWidget *parent)
: QDialog(parent),
  ui(new Ui::ConfigDialog)
{
    this->ui->setupUi(this);

    connect(this->ui->buttonBox, &QDialogButtonBox::clicked, this, &ConfigDialog::buttonBoxClicked);
}

ConfigDialog::~ConfigDialog()
{
    delete ui;
}

void ConfigDialog::fillProperties()
{
    bool oldState = this->ui->comboBoxAudioDriver->blockSignals(true);
    this->ui->comboBoxAudioDriver->clear();
    for (int i = static_cast<int>(AudioDriver_t::BEGIN); i < static_cast<int>(AudioDriver_t::END); i++)
    {
        this->ui->comboBoxAudioDriver->insertItem(i, AudioDriverName[i]);
    }
    this->ui->comboBoxAudioDriver->blockSignals(oldState);
    this->ui->comboBoxAudioDriver->setCurrentIndex(static_cast<int>(this->newConfig.audioDriver));

    this->ui->spinPreRenderTime->setValue(this->newConfig.PreRenderTime);
    this->ui->checkAudioNorm->setChecked(this->newConfig.useAudioNormalization);
    this->ui->checkRenderWhole->setChecked(this->newConfig.RenderWholeSong);
    this->ui->checkMADVFREE->setChecked(this->newConfig.useMadvFree);

    this->ui->checkLoopInfo->setChecked(this->newConfig.useLoopInfo);
    this->ui->spinFadePause->setValue(this->newConfig.fadeTimePause);
    this->ui->spinFadeStop->setValue(this->newConfig.fadeTimeStop);
    this->ui->spinLoopCount->setValue(this->newConfig.overridingGlobalLoopCount);

    this->ui->checkUsfHle->setChecked(this->newConfig.useHle);

    this->ui->spinGmeSampleRate->setValue(this->newConfig.gmeSampleRate);
    this->ui->spinGmeSfx->setValue(this->newConfig.gmeEchoDepth);
    this->ui->checkGmeAccurate->setChecked(this->newConfig.gmeAccurateEmulation);
    this->ui->checkGmeForever->setChecked(this->newConfig.gmePlayForever);
    this->ui->checkGmeMultiChannel->setChecked(this->newConfig.gmeMultiChannel);

    this->ui->spinFluidSampleRate->setValue(this->newConfig.FluidsynthSampleRate);
    this->ui->checkMultiChannel->setChecked(this->newConfig.FluidsynthMultiChannel);
    this->ui->defaultSF2Path->setText(QString::fromStdString(this->newConfig.FluidsynthDefaultSoundfont));
    this->ui->checkForceDefaultSf->setChecked(this->newConfig.FluidsynthForceDefaultSoundfont);
    this->ui->checkReverb->setChecked(this->newConfig.FluidsynthEnableReverb);
    this->ui->spinRoomSize->setValue(this->newConfig.FluidsynthRoomSize);
    this->ui->spinDamping->setValue(this->newConfig.FluidsynthDamping);
    this->ui->spinWidth->setValue(this->newConfig.FluidsynthWidth);
    this->ui->spinLevel->setValue(this->newConfig.FluidsynthLevel);
    this->ui->checkChorus->setChecked(this->newConfig.FluidsynthEnableChorus);
    this->ui->checkChorus->setChecked(this->newConfig.FluidsynthEnableChorus);
    this->ui->checkChan9Drum->setChecked(this->newConfig.FluidsynthChannel9IsDrum);
    this->ui->comboBankSelMode->setCurrentText(QString::fromStdString(this->newConfig.FluidsynthBankSelect));
    this->ui->spinFilterQ->setValue(this->newConfig.FluidsynthFilterQ);
    this->ui->checkRenderNoPresetAssigned->setChecked(this->newConfig.FluidsynthRenderNotesWithoutPreset);

    this->ui->checkModBass->setChecked(this->newConfig.ModPlugEnableBass);
    this->ui->checkModNoise->setChecked(this->newConfig.ModPlugEnableNoiseRed);
    this->ui->checkModReverb->setChecked(this->newConfig.ModPlugEnableReverb);
    this->ui->checkModSurround->setChecked(this->newConfig.ModPlugEnableSurround);
    this->ui->spinModBassAmount->setValue(this->newConfig.ModPlugBassAmount);
    this->ui->spinModBassRange->setValue(this->newConfig.ModPlugBassRange);
    this->ui->spinModReverbDelay->setValue(this->newConfig.ModPlugReverbDelay);
    this->ui->spinModReverbDepth->setValue(this->newConfig.ModPlugReverbDepth);
    this->ui->spinModSampleRate->setValue(this->newConfig.ModPlugSampleRate);
    this->ui->spinModSurroundDelay->setValue(this->newConfig.ModPlugSurroundDelay);
    this->ui->spinModSurroundDepth->setValue(this->newConfig.ModPlugSurroundDepth);
}


void ConfigDialog::showEvent(QShowEvent *event)
{
    // only do this if the dialog wasnt shown before, i.e. ignore minimize
    if (!this->isShown)
    {
        this->newConfig = gConfig;
        this->fillProperties();
    }

    this->isShown = true;
    this->QDialog::showEvent(event);
}

void ConfigDialog::closeEvent(QCloseEvent *event)
{
    this->isShown = false;
    this->QDialog::closeEvent(event);
}

void ConfigDialog::accept()
{
    this->isShown = false;
    this->QDialog::accept();
}

void ConfigDialog::done(int r)
{
    this->isShown = false;
    this->QDialog::done(r);
}

void ConfigDialog::on_comboBoxAudioDriver_currentIndexChanged(int index)
{
    this->newConfig.audioDriver = static_cast<AudioDriver_t>(index);
    /** in case we ever use bitmasks for audioDriver

        if(this->newConfig.audioDriver & (AudioDriver_t::Wave | AudioDriver_t::Ebur128))
        {
            this->newConfig.RenderWholeSong = false;
            this->ui->checkRenderWhole->setEnabled(false);
            this->ui->checkRenderWhole->setChecked(false);

            if(this->newConfig.audioDriver & AudioDriver_t::Ebur128)
            {
                this->newConfig.useAudioNormalization = false;
                this->ui->checkAudioNorm->setEnabled(false);
                this->ui->checkAudioNorm->setChecked(false);
            }
        }
        else
        {
            this->ui->checkRenderWhole->setEnabled(true);
            this->ui->checkAudioNorm->setEnabled(true);
        }*/

    switch (this->newConfig.audioDriver)
    {
#ifdef USE_EBUR128
        case AudioDriver_t::Ebur128:
            this->ui->checkAudioNorm->setEnabled(false);
            this->ui->checkAudioNorm->setChecked(false);

            this->ui->checkLoopInfo->setEnabled(false);
            this->ui->checkLoopInfo->setChecked(false);

            this->ui->checkRenderWhole->setEnabled(false);
            this->ui->checkRenderWhole->setChecked(false);
            break;
#endif
        case AudioDriver_t::Wave:
            this->ui->checkRenderWhole->setEnabled(false);
            this->ui->checkRenderWhole->setChecked(false);

            this->ui->checkAudioNorm->setEnabled(false);
            this->ui->checkAudioNorm->setChecked(false);

            this->ui->checkLoopInfo->setEnabled(true);
            this->ui->checkLoopInfo->setChecked(this->newConfig.useLoopInfo);
            break;

        default:
            this->ui->checkRenderWhole->setEnabled(true);
            this->ui->checkRenderWhole->setChecked(this->newConfig.RenderWholeSong);

            this->ui->checkAudioNorm->setEnabled(true);
            this->ui->checkAudioNorm->setChecked(this->newConfig.useAudioNormalization);

            this->ui->checkLoopInfo->setEnabled(true);
            this->ui->checkLoopInfo->setChecked(this->newConfig.useLoopInfo);
            break;
    }
}

void ConfigDialog::on_browseSF2_clicked()
{
    QString selFilter = "SoundFonts (*.sf2 *.sf3 *.dls) (*.sf2 *.sf3 *.dls)";
    QString sf2 = QFileDialog::getOpenFileName(this, "Select Soundfont", this->ui->defaultSF2Path->text(), selFilter + ";;All files (*.*)", &selFilter);

    if (!sf2.isNull())
    {
        this->ui->defaultSF2Path->setText(sf2);
    }
}

void ConfigDialog::buttonBoxClicked(QAbstractButton *btn)
{
    QDialogButtonBox::StandardButton stdbtn = this->ui->buttonBox->standardButton(btn);

    if (stdbtn == QDialogButtonBox::Cancel)
    {
        return;
    }

    if (stdbtn == QDialogButtonBox::RestoreDefaults)
    {
        this->newConfig = Config();
        this->fillProperties();
        return;
    }

    if (stdbtn == QDialogButtonBox::Save || stdbtn == QDialogButtonBox::Ok)
    {
        this->newConfig.RenderWholeSong = this->ui->checkRenderWhole->isChecked();
        this->newConfig.useAudioNormalization = this->ui->checkAudioNorm->isChecked();
        this->newConfig.PreRenderTime = static_cast<unsigned int>(this->ui->spinPreRenderTime->value());
        this->newConfig.useMadvFree = this->ui->checkMADVFREE->isChecked();

        this->newConfig.useLoopInfo = this->ui->checkLoopInfo->isChecked();
        this->newConfig.overridingGlobalLoopCount = this->ui->spinLoopCount->value();
        this->newConfig.fadeTimeStop = this->ui->spinFadeStop->value();
        this->newConfig.fadeTimePause = this->ui->spinFadePause->value();

        this->newConfig.useHle = this->ui->checkUsfHle->isChecked();

        this->newConfig.gmeEchoDepth = static_cast<float>(this->ui->spinGmeSfx->value());
        this->newConfig.gmeSampleRate = static_cast<unsigned int>(this->ui->spinGmeSampleRate->value());
        this->newConfig.gmeAccurateEmulation = this->ui->checkGmeAccurate->isChecked();
        this->newConfig.gmePlayForever = this->ui->checkGmeForever->isChecked();
        this->newConfig.gmeMultiChannel = this->ui->checkGmeMultiChannel->isChecked();

        this->newConfig.FluidsynthSampleRate = static_cast<unsigned int>(this->ui->spinFluidSampleRate->value());
        this->newConfig.FluidsynthMultiChannel = this->ui->checkMultiChannel->isChecked();
        this->newConfig.FluidsynthDefaultSoundfont = this->ui->defaultSF2Path->text().toUtf8().constData();
        this->newConfig.FluidsynthForceDefaultSoundfont = this->ui->checkForceDefaultSf->isChecked();
        this->newConfig.FluidsynthEnableReverb = this->ui->checkReverb->isChecked();
        this->newConfig.FluidsynthRoomSize = this->ui->spinRoomSize->value();
        this->newConfig.FluidsynthDamping = this->ui->spinDamping->value();
        this->newConfig.FluidsynthWidth = this->ui->spinWidth->value();
        this->newConfig.FluidsynthLevel = this->ui->spinLevel->value();
        this->newConfig.FluidsynthEnableChorus = this->ui->checkChorus->isChecked();
        this->newConfig.FluidsynthChannel9IsDrum = this->ui->checkChan9Drum->isChecked();
        this->newConfig.FluidsynthBankSelect = this->ui->comboBankSelMode->currentText().toStdString();
        this->newConfig.FluidsynthFilterQ = this->ui->spinFilterQ->value();
        this->newConfig.FluidsynthRenderNotesWithoutPreset = this->ui->checkRenderNoPresetAssigned->isChecked();

        this->newConfig.ModPlugEnableBass = this->ui->checkModBass->isChecked();
        this->newConfig.ModPlugEnableNoiseRed = this->ui->checkModNoise->isChecked();
        this->newConfig.ModPlugEnableReverb = this->ui->checkModReverb->isChecked();
        this->newConfig.ModPlugEnableSurround = this->ui->checkModSurround->isChecked();
        this->newConfig.ModPlugBassAmount = this->ui->spinModBassAmount->value();
        this->newConfig.ModPlugBassRange = this->ui->spinModBassRange->value();
        this->newConfig.ModPlugReverbDelay = this->ui->spinModReverbDelay->value();
        this->newConfig.ModPlugReverbDepth = this->ui->spinModReverbDepth->value();
        this->newConfig.ModPlugSampleRate = this->ui->spinModSampleRate->value();
        this->newConfig.ModPlugSurroundDelay = this->ui->spinModSurroundDelay->value();
        this->newConfig.ModPlugSurroundDepth = this->ui->spinModSurroundDepth->value();

        std::swap(gConfig, this->newConfig);
        if (stdbtn == QDialogButtonBox::Save)
        {
            gConfig.Save();
        }
    }
}
