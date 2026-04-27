
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
        return static_cast<int>(min + ((max - min) * 1.0 * (height - event->position().y())) / height);
    }
    else
    {
        return static_cast<int>(min + ((max - min) * 1.0 * event->position().x()) / width);
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
        
        QToolTip::showText(event->globalPosition().toPoint(),
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
    Q_ASSERT(option.orientation == Qt::Horizontal);

    // Handle rect at current playhead position
    const QRect playheadHandle = this->style()->subControlRect(QStyle::CC_Slider,
                                            &option,
                                            QStyle::SC_SliderHandle,
                                            this);

    // Handle rect at buffer health position
    option.sliderPosition = this->bufferHealth;
    const QRect bufferHandle = this->style()->subControlRect(QStyle::CC_Slider,
                                            &option,
                                            QStyle::SC_SliderHandle,
                                            this);

    // -----------------------------
    // Render yellow area: from groove start up to buffer health
    //
    const QRect yellowBox = QRect(
        QPoint(groove.left(), groove.top()),
        QPoint(bufferHandle.center().x(), groove.bottom()))
        .intersected(groove);

    if (!yellowBox.isEmpty())
    {
        QLinearGradient yellowGradient(yellowBox.left(), groove.center().y(),
                                       yellowBox.right(), groove.center().y());
        yellowGradient.setColorAt(0, QColor(255, 255, 180));
        yellowGradient.setColorAt(1, QColor(255, 244, 105));
        p.setPen(Qt::NoPen);
        p.setBrush(yellowGradient);
        p.drawRect(yellowBox);
    }

    // -----------------------------
    // Render green area: already-played range (groove start to current playhead)
    //
    const QRect greenBox = QRect(
        QPoint(groove.left(), groove.top()),
        QPoint(playheadHandle.left(), groove.bottom()))
        .intersected(groove);

    if (!greenBox.isEmpty())
    {
        QLinearGradient greenGradient(greenBox.left(), groove.center().y(),
                                      greenBox.right(), groove.center().y());
        greenGradient.setColorAt(0, QColor(180, 255, 180));
        greenGradient.setColorAt(1, QColor(105, 225, 105));
        p.setPen(Qt::NoPen);
        p.setBrush(greenGradient);
        p.drawRect(greenBox);
    }

    this->initStyleOption(&option);
    option.subControls = QStyle::SC_SliderHandle;
    if(this->mouseIsOver)
    {
        option.activeSubControls = QStyle::SC_SliderHandle;
    }
    p.drawComplexControl(QStyle::CC_Slider, option);

    constexpr qreal MarkerPixelWidth = 2;
    QPen loopMarkerPen(QBrush(Qt::black), MarkerPixelWidth, Qt::DotLine);
    p.setPen(loopMarkerPen);
    for(const auto& currentLoop : this->currentLoops)
    {
        option.sliderPosition = currentLoop.start;
        const QRect lowerRange = this->style()->subControlRect(QStyle::CC_Slider,
                                                &option,
                                                QStyle::SC_SliderHandle,
                                                this);

        // draw loop start marker
        p.drawLine(QPoint(lowerRange.center().x(), groove.top()),
                   QPoint(lowerRange.center().x(), groove.bottom()));

        option.sliderPosition = currentLoop.stop;
        const QRect upperRange = this->style()->subControlRect(QStyle::CC_Slider,
                                                &option,
                                                QStyle::SC_SliderHandle,
                                                this);
        // draw loop stop marker
        p.drawLine(QPoint(upperRange.center().x(), groove.top()),
                   QPoint(upperRange.center().x(), groove.bottom()));
    }
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
        this->currentLoops = s->getLoopArray();
        this->setMaximum(s->getFrames());
    }
}

void PlayheadSlider::setBufferHealth(long long frames)
{
    this->bufferHealth = frames;
    this->update();
}
