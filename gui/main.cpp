#include "anmp.hpp"
#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>
#include <QFileInfo>
#include <QUrl>

#ifdef USE_DBUS
#include <QtDBus>
#include <QDBusConnectionInterface>
#endif

using namespace std;

int main(int argc, char *argv[])
{
    int ret = -1;

#ifdef USE_DBUS
    constexpr char kMprisServiceName[] = "org.mpris.MediaPlayer2.anmp";
    constexpr char kMprisObjectPath[] = "/org/mpris/MediaPlayer2";
    constexpr char kMprisTrackListIface[] = "org.mpris.MediaPlayer2.TrackList";
#endif

    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));
    QMessageBox msgBox;

#ifdef USE_DBUS
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
        reply = dbus.interface()->isServiceRegistered(QString::fromUtf8(kMprisServiceName));
    }
    
    if (
#if !defined(NDEBUG)
        true ||
#endif
        (reply.isValid() && !reply.value()))
#endif
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
#ifdef USE_DBUS
    else // anmp already started, feed songs via dbus
    {
        QDBusInterface interface(QString::fromUtf8(kMprisServiceName), QString::fromUtf8(kMprisObjectPath), QString::fromUtf8(kMprisTrackListIface), dbus, &a);

        QStringList fileList;
        for (int i = 1; i < argc; i++)
        {
            QFileInfo info(argv[i]);
            fileList.append(info.absoluteFilePath());
        }

        for (const QString &file : fileList)
        {
            interface.call(QStringLiteral("AddTrack"), QUrl::fromLocalFile(file).toString(), QDBusObjectPath(QStringLiteral("/")), false);
        }
    }
#endif

    return ret;
}
