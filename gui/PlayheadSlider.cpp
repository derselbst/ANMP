
#include "PlayheadSlider.h"
#include "Song.h"
#include "Common.h"

#include <QMouseEvent>
#include <QToolTip>

PlayheadSlider::PlayheadSlider(QWidget *parent)
: QSlider(parent)
{
    // receive mouse move events even if no buttons are pressed
    this->setMouseTracking(true);
}

int PlayheadSlider::getFrameFromMouseEvt(const QMouseEvent *event)
{
    const int min = this->minimum();
    const int max = this->maximum();
    const int height = this->height();
    const int width = this->width();

    if (this->orientation() == Qt::Vertical)
    {
        return static_cast<int>(min + ((max - min) * 1.0 * (height - event->y())) / height);
    }
    else
    {
        return static_cast<int>(min + ((max - min) * 1.0 * event->x()) / width);
    }
}


void PlayheadSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        int sliderPos = getFrameFromMouseEvt(event);
        emit this->sliderMoved(sliderPos);

        event->accept();
    }

    QSlider::mousePressEvent(event);
}

void PlayheadSlider::mouseMoveEvent(QMouseEvent *event)
{
    if(this->currentSampleRate != 0)
    {
        const QPoint pos = event->pos();
        const QRect rect(pos.x(), pos.y(), 1, this->height());
        int frameOnSlider = getFrameFromMouseEvt(event);
        
        
        QToolTip::showText(event->globalPos(),
                           QString::fromStdString(framesToTimeStr(frameOnSlider, this->currentSampleRate)),
                           this,
                           rect);
        event->accept();
    }
    
    QSlider::mouseMoveEvent(event);
}

void PlayheadSlider::SilentReset()
{
    bool oldState = this->blockSignals(true);
    this->setSliderPosition(0);
    this->setMaximum(0);
    this->blockSignals(oldState);
}

void PlayheadSlider::slotCurrentSongChanged(const Song *s)
{
    if (s == nullptr || !s->Format.IsValid())
    {
        this->currentSampleRate = 0;
    }
    else
    {
        this->currentSampleRate = s->Format.SampleRate;
    }
}
