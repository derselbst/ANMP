#include "anmp.hpp"
#include "anmp_dbus_interface.h"
#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QtDBus>
#include <QDBusConnectionInterface>

using namespace std;

int main(int argc, char *argv[])
{
    int ret = -1;

    QApplication a(argc, argv);
    QMessageBox msgBox;

    QDBusReply<bool> reply;
    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (!dbus.isConnected())
    {
        constexpr char dbusErr[] = "Cannot connect to the D-Bus session bus.\n"
                                  "Please check your system settings and try again.\n";
        CLOG(LogLevel_t::Warning, dbusErr);
        
        msgBox.setText(dbusErr);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }
    else
    {
        reply = dbus.interface()->isServiceRegistered(org::anmp::staticInterfaceName());
    }
    
    if (
#if !defined(NDEBUG)
        true ||
#endif
        (reply.isValid() && !reply.value()))
    {
        CLOG(LogLevel_t::Debug, "ANMP not started yet");
        
        gConfig.Load();
        MainWindow w;
        
        try
        {
            QStringList fileList;
            for (int i = 1; i < argc; i++)
            {
                QFileInfo info(argv[i]);
                fileList.append(info.absoluteFilePath());
            }

            w.AddSongs(fileList);
            w.show();
            
            ret = a.exec();
        }
        catch (const logic_error &e)
        {
            w.hide();

            CLOG(LogLevel_t::Fatal, "Terminated after throwing logic_error: " << e.what());

            msgBox.setText("You've discovered a bug in some logic implementation. Please let us know the details below.");
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setDetailedText(e.what());
            msgBox.exec();
        }
        catch (const runtime_error &e)
        {
            w.hide();

            CLOG(LogLevel_t::Fatal, "Terminated after throwing runtime_error: " << e.what());

            msgBox.setText("An unhandled runtime error occurred, probably a bug. Please let us know the details below.");
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setDetailedText(e.what());
            msgBox.exec();
        }
        catch (const exception &e)
        {
            w.hide();

            CLOG(LogLevel_t::Fatal, "Terminated after throwing this: " << e.what());

            msgBox.setText("Some kind of exception occurred, likely a bug. Please let us know the details below.");
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setDetailedText(e.what());
            msgBox.exec();
        }
        catch (...) // for proper stack unwinding
        {
            w.hide();

            CLOG(LogLevel_t::Fatal, "Terminated after throwing ???");

            QMessageBox msgBox;
            msgBox.setText("An unknown error occurred. If you can reproduce this, please let us know.");
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();
        }
    }
    else // anmp already started, feed songs via dbus
    {
        constexpr char Path[] = "/MainWindow";
        org::anmp *interface = new org::anmp(org::anmp::staticInterfaceName(), Path, dbus, &a);

        QStringList fileList;
        for (int i = 1; i < argc; i++)
        {
            QFileInfo info(argv[i]);
            fileList.append(info.absoluteFilePath());
        }

        interface->AddSongs(fileList);
    }

    return ret;
}
