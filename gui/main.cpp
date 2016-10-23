#include "mainwindow.h"
#include "AtomicWrite.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    int ret;
    try
    {
        ret = a.exec();
    }
    catch(const exception& e)
    {
        w.hide();

        CLOG(LogLevel::FATAL, "Terminated after throwing this: " << e.what());

        QMessageBox msgBox;
        msgBox.setText("The Playback unexpectedly stopped.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setDetailedText(e.what());
        msgBox.exec();

        ret = -1;
    }

    return ret;
}
