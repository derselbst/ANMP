
#pragma once

#include <QSlider>

class PlayheadSlider : public QSlider
{
    Q_OBJECT

    public:
    PlayheadSlider(QWidget *parent);

    void SilentReset();

    protected:
    void mousePressEvent(QMouseEvent *event) override;
};
