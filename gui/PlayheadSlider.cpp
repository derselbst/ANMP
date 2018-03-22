
#include "PlayheadSlider.h"

#include <QMouseEvent>

PlayheadSlider::PlayheadSlider(QWidget *parent)
: QSlider(parent)
{
}

void PlayheadSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        const int min = this->minimum();
        const int max = this->maximum();
        const int height = this->height();
        const int width = this->width();

        if (this->orientation() == Qt::Vertical)
        {
            emit this->sliderMoved(min + ((max - min) * (height - event->y())) / height);
        }
        else
        {
            int newVal = static_cast<int>(min + ((max - min) * 1.0 * event->x()) / width);
            emit this->sliderMoved(newVal);
        }

        event->accept();
    }

    QSlider::mousePressEvent(event);
}

void PlayheadSlider::SilentReset()
{
    bool oldState = this->blockSignals(true);
    this->setSliderPosition(0);
    this->setMaximum(0);
    this->blockSignals(oldState);
}
