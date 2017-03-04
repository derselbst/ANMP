#include "mainwindow.h"
#include "AtomicWrite.h"
#include "Config.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    gConfig.Load();
    
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    int ret=-1;
    try
    {
        ret = a.exec();
    }
    catch(const logic_error& e)
    {
        w.hide();

        CLOG(LogLevel_t::Fatal, "Terminated after throwing logic_error: " << e.what());

        QMessageBox msgBox;
        msgBox.setText("You've discovered a bug in some logic implementation. Please let us know the details below.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setDetailedText(e.what());
        msgBox.exec();
    }
    catch(const runtime_error& e)
    {
        w.hide();

        CLOG(LogLevel_t::Fatal, "Terminated after throwing runtime_error: " << e.what());

        QMessageBox msgBox;
        msgBox.setText("An unhandled runtime error occurred, probably a bug. Please let us know the details below.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setDetailedText(e.what());
        msgBox.exec();
    }
    catch(const exception& e)
    {
        w.hide();

        CLOG(LogLevel_t::Fatal, "Terminated after throwing this: " << e.what());

        QMessageBox msgBox;
        msgBox.setText("Some kind of exception occurred, likely a bug. Please let us know the details below.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setDetailedText(e.what());
        msgBox.exec();
    }
    catch(...) // for proper stack unwinding
    {
        w.hide();

        CLOG(LogLevel_t::Fatal, "Terminated after throwing ???");

        QMessageBox msgBox;
        msgBox.setText("An unknown error occurred. If you can reproduce this, please let us know.");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
    }

    return ret;
}
