#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>

namespace Ui
{
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigDialog(QWidget *parent = 0);
    ~ConfigDialog();

private slots:
    void on_checkRenderWhole_clicked(bool checked);

    void on_checkAudioNorm_clicked(bool checked);

    void on_spinPreRenderTime_valueChanged(int arg1);

    void on_checkLoopInfo_clicked(bool checked);

    void on_spinLoopCount_valueChanged(int arg1);

    void on_spinFadeStop_valueChanged(int arg1);

    void on_spinFadePause_valueChanged(int arg1);

private:
    Ui::ConfigDialog *ui;
};

#endif // CONFIGDIALOG_H
