
#pragma once

#include <QSlider>

class Song;
class QStylePainter;

class PlayheadSlider : public QSlider
{
    Q_OBJECT

    public:
    PlayheadSlider(QWidget *parent);
    void SlotCurrentSongChanged(const Song *s);
    void setBufferHealth(long long framesRendered);

    void SilentReset();

    protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent* ev) override;
    
    int getFrameFromMouseEvt(const QMouseEvent *event);
    
    
    private:
    uint32_t currentSampleRate=0;
    long long bufferHealth=0;
};
