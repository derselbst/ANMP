#ifndef OVERLAYCONTROL_H
#define OVERLAYCONTROL_H

#include <QDialog>

namespace Ui {
class OverlayControl;
}

class OverlayControl : public QDialog
{
    Q_OBJECT

public:
    explicit OverlayControl(QWidget *parent = 0);
    ~OverlayControl();

private:
    Ui::OverlayControl *ui;
};

#endif // OVERLAYCONTROL_H
