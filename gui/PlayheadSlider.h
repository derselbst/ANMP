
#pragma once

#include <QSlider>

class Song;

class PlayheadSlider : public QSlider
{
    Q_OBJECT

    public:
    PlayheadSlider(QWidget *parent);

    void SilentReset();

    protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    
    int getFrameFromMouseEvt(const QMouseEvent *event);
    
    
    protected slots:
    void slotCurrentSongChanged(const Song *s);
    
    private:
    uint32_t currentSampleRate=0;
};
