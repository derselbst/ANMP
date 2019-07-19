
#pragma once

#include <QSlider>
#include <vector>
#include "types.h"

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
    bool event(QEvent *event) override;

    int getFrameFromMouseEvt(const QMouseEvent *event);

    private:
    uint32_t currentSampleRate=0;
    long long bufferHealth=0;
    std::vector<loop_t> currentLoops;
    bool mouseIsOver=false;
};
