
#pragma once

#include <QSlider>
#include <mutex>

class MainWindow;

class PlayheadSlider : public QSlider
{
    Q_OBJECT

public:
    PlayheadSlider( QWidget* parent );
    
    void SetMainWindow(MainWindow* wnd);
    
protected:
    void mousePressEvent(QMouseEvent * event) override;
    
private:
    mutable std::mutex mtx;
    MainWindow* mainWnd = nullptr;
    
};