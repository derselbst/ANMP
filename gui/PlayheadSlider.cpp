
#include "PlayheadSlider.h"
#include "Song.h"
#include "Common.h"

#include <QMouseEvent>
#include <QToolTip>
#include <QStyleOptionSlider>
#include <QStylePainter>
#include <QStyle>

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

bool PlayheadSlider::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::HoverEnter:
        this->mouseIsOver = true;
        break;
    case QEvent::HoverLeave:
        this->mouseIsOver = false;
    default:
        break;
    }
    return QSlider::event(event);
}

void PlayheadSlider::paintEvent( QPaintEvent* )
{
    QStylePainter p(this);
    QStyleOptionSlider option;
    this->initStyleOption(&option);

    option.subControls = QStyle::SC_SliderGroove;
    p.drawComplexControl(QStyle::CC_Slider, option);

    QRect groove = this->style()->subControlRect(QStyle::CC_Slider,
                                                    &option,
                                                    QStyle::SC_SliderGroove,
                                                    this );
    QRect rangeBox;
    if (option.orientation == Qt::Horizontal)
    {
        const QRect lowerRange = this->style()->subControlRect(QStyle::CC_Slider,
                                                &option,
                                                QStyle::SC_SliderHandle,
                                                this);

        option.sliderPosition = this->bufferHealth;
        const QRect upperRange = this->style()->subControlRect(QStyle::CC_Slider,
                                                &option,
                                                QStyle::SC_SliderHandle,
                                                this);

        rangeBox = QRect(
        QPoint(lowerRange.left(), groove.top()),
        QPoint(upperRange.center().x(), groove.bottom()));
    }
    else
    {
        Q_ASSERT(false);
    }

    // -----------------------------
    // Render the range
    //
    rangeBox = rangeBox.intersected(groove);

    QLinearGradient gradient;
    if (option.orientation == Qt::Horizontal)
    {
        gradient = QLinearGradient( rangeBox.left(), groove.center().y(),
                                rangeBox.right(), groove.center().y());
    }
    else
    {
        Q_ASSERT(false);
    }

    const QColor l = QColor(255,255,180);
    const QColor u = QColor(255,244,105);

    gradient.setColorAt(0, l);
    gradient.setColorAt(1, u);

    p.setPen(QPen(Qt::white, 0));
    p.setBrush(gradient);
    p.drawRect( rangeBox.intersected(groove) );

    this->initStyleOption(&option);
    option.subControls = QStyle::SC_SliderHandle;
    if(this->mouseIsOver)
    {
        option.activeSubControls = QStyle::SC_SliderHandle;
    }
    p.drawComplexControl(QStyle::CC_Slider, option);
}

void PlayheadSlider::SilentReset()
{
    bool oldState = this->blockSignals(true);
    this->currentSampleRate = 0;
    this->setSliderPosition(0);
    this->setMaximum(0);
    this->setBufferHealth(0);
    this->blockSignals(oldState);
}

void PlayheadSlider::SlotCurrentSongChanged(const Song *s)
{
    if (s == nullptr || !s->Format.IsValid())
    {
        this->SilentReset();
    }
    else
    {
        this->currentSampleRate = s->Format.SampleRate;
        this->setMaximum(s->getFrames());
    }
}

void PlayheadSlider::setBufferHealth(long long frames)
{
    this->bufferHealth = frames;
}
