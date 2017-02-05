
#include "PlayheadSlider.h"
#include "mainwindow.h"

#include <QMouseEvent>

PlayheadSlider::PlayheadSlider( QWidget *parent ) : QSlider(parent)
{}

void PlayheadSlider::SetMainWindow(MainWindow* wnd)
{
    lock_guard<std::mutex> lck(this->mtx);
    this->mainWnd = wnd;
}

void PlayheadSlider::mousePressEvent(QMouseEvent * event)
{
    if (event->button() == Qt::LeftButton)
    {
        lock_guard<std::mutex> lck(this->mtx);

        if(this->mainWnd != nullptr)
        {
            const int min = this->minimum();
            const int max = this->maximum();
            const int height = this->height();
            const int width = this->width();
            
            if (this->orientation() == Qt::Vertical)
            {
                this->mainWnd->on_seekBar_sliderMoved(min + ((max-min) * (height - event->y())) / height);
            }
            else
            {
                int newVal = static_cast<int>(min + ((max-min) * 1.0 * event->x()) / width);
                this->mainWnd->on_seekBar_sliderMoved(newVal);
            }

            event->accept();
        }
    }
    
    QSlider::mousePressEvent(event);
}