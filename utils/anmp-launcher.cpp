// anmp-launcher: appends songs to the currently running anmp instance via dbus

#include "anmp_dbus_interface.h"

#include <QApplication>
#include <QtDBus>
#include <QMessageBox>
#include <QDBusConnectionInterface>
#include <iostream>

int main(int argc, char *argv[])
{
    if(argc <= 1)
    {
        return 0;
    }
    
    QApplication a(argc, argv);
        
    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (!dbus.isConnected())
    {
        std::cerr << "Cannot connect to the D-Bus session bus.\n""Please check your system settings and try again." << std::endl;
        return -1;
    }
    
    QDBusReply<bool> reply = dbus.interface()->isServiceRegistered(org::anmp::staticInterfaceName());
    if(reply.isValid() && reply.value() == false)
    {
        std::cout << "anmp not started yet" << std::endl;
        QMessageBox box(QMessageBox::Warning, "ANMP not started yet", "ANMP has not yet registered a dbus service, probably not started.", QMessageBox::Ok);
        box.exec();
        return -1;
    }
    
    constexpr char Path[] = "/MainWindow";
    org::anmp* interface = new org::anmp(org::anmp::staticInterfaceName(), Path, dbus, &a);
    
    QStringList fileList;
    for(int i=1; i<argc; i++)
    {
        fileList.append(argv[i]);
    }
    
    interface->AddSongs(fileList);
    
    return 0;
}
 
