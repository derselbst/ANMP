#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <anmp.hpp>

class QAbstractButton;
class Config;

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

    // while this dialog is opened: contains the new settings made by the user. might be discarded if dialog gets cancelled
    // once the dialog gets accepted: holds the old settings from gConfig
    Config newConfig;

private slots:

    void on_comboBoxAudioDriver_currentIndexChanged(int index);

    void on_browseSF2_clicked();

    void buttonBoxClicked(QAbstractButton* btn);

private:
    Ui::ConfigDialog *ui;
    void fillProperties();
};

#endif // CONFIGDIALOG_H
