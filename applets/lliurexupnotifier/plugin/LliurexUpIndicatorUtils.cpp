/*
 * Copyright (C) 2023 Juan Ramon Pelegrina <juapesai@hotmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "LliurexUpIndicatorUtils.h"

#include <QFile>
#include <QDateTime>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>
#include <QTextStream>
#include <QTimer>
#include <QEventLoop>

#include <QPointer>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusVariant>

#include <QtConcurrent/QtConcurrentRun>

#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>

#include <n4d.hpp>

using namespace edupals;

LliurexUpIndicatorUtils::LliurexUpIndicatorUtils(QObject *parent)
    : QObject(parent)
       
{
    user=qgetenv("USER");
    PKGCACHE.setFileName("/var/cache/apt/pkgcache.bin");
    AUTO_UPDATE_TOKEN.setFileName("/var/run/lliurex-up-auto-upgrade.token");
    AUTO_UPDATE_RUN_TOKEN.setFileName("/var/run/lliurex-up-auto-upgrade.lock");

}

void LliurexUpIndicatorUtils::startWidget(){

    QPointer<LliurexUpIndicatorUtils>safeThis(this);

    QtConcurrent::run([safeThis]() {

        if (!safeThis){
            return;
        }

        bool hideWidget=false;
        bool isClient=false;
        bool isDesktop=false;

        QStringList flavours=safeThis->getFlavours();
        QStringList userGroups=safeThis->getUserGroups();

        if (userGroups.length()>0){
            if (!flavours.contains("None")){
                for (int i=0;i<flavours.count();i++){
                    if (flavours[i].contains("client")){
                        isClient=true; 
                    }
                    if (flavours[i].contains("desktop")){
                        isDesktop=true;
                    }
                    if (flavours[i].contains("server")){
                        if (userGroups.contains("teachers")){
                            if ((!userGroups.contains("sudo")&&(!userGroups.contains("admins")))){
                                hideWidget=true;
                                break;
                            }
                        }
                    }
                }
            }
        }else{
            if (!safeThis->isStudent){
                hideWidget=true;
            }
        }

        if (isClient){
            if (isDesktop){
                if (safeThis->isConnectionWithServer()){
                    hideWidget=true;
                }
            }else{
                hideWidget=true;
            }
        }

        if (safeThis){
            emit safeThis->startWidgetFinished(hideWidget);
        }
    });

}

QStringList LliurexUpIndicatorUtils::getFlavours(){

    QProcess process;
    QStringList flavours;
    process.start(QStringLiteral("lliurex-version"), QStringList() << QStringLiteral("-v"));
    process.waitForFinished(-1);

    if (process.waitForFinished(-1)) {
        QString stdout = QString::fromLocal8Bit(process.readAllStandardOutput());
        flavours = stdout.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    }

    return flavours;

}

QStringList LliurexUpIndicatorUtils::getUserGroups(){
    
    int j, ngroups=32;
    gid_t groups[32];
    struct passwd *pw;
    struct group *gr;
    QStringList userGroups;
    isStudent=false;


    QByteArray uname = user.toLocal8Bit();
    const char *username = uname.data();
    pw=getpwnam(username);
    getgrouplist(username, pw->pw_gid, groups, &ngroups);
    for (j = 0; j < ngroups; j++) {
        gr = getgrgid(groups[j]);
        if (gr != NULL){
            if ((strcmp(gr->gr_name,"adm")==0)||(strcmp(gr->gr_name,"admins")==0)||(strcmp(gr->gr_name,"teachers")==0)){
                userGroups.append(gr->gr_name);
            }else if (strcmp(gr->gr_name,"students")==0){
                isStudent=true;
            } 
        }  
    }
    return userGroups;
}    

bool LliurexUpIndicatorUtils::runUpdateCache() {
    
    QDBusConnection bus = QDBusConnection::systemBus();
    
    if (!bus.isConnected()) return false;

    if (cacheUpdated) {
        cacheUpdated = false;
        return simulateUpgrade();
    }

    QDBusInterface aptIface("org.debian.apt", "/org/debian/apt", "org.debian.apt", bus);
    QString trans = aptIface.call("UpdateCache").arguments().value(0).toString();
    if (trans.isEmpty()) return false;

    QEventLoop loop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    
    bus.connect("org.debian.apt", trans, "org.freedesktop.DBus.Properties", "PropertiesChanged", &loop, SLOT(quit()));
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    QDBusInterface transIface("org.debian.apt", trans, "org.debian.apt.transaction", bus);
    transIface.asyncCall("Run");
    timeoutTimer.start(300000);

    QDBusInterface propIface("org.debian.apt", trans, "org.freedesktop.DBus.Properties", bus);
    while (timeoutTimer.isActive()) {
        QString state = propIface.call("Get", "org.debian.apt.transaction", "ExitState")
                        .arguments().value(0).value<QDBusVariant>().variant().toString();
        
        if (state != "exit-unfinished") break; 
        loop.exec(); 
    }

    return simulateUpgrade();
}

bool LliurexUpIndicatorUtils::simulateUpgrade() {

    QDBusConnection bus = QDBusConnection::systemBus();
    QDBusInterface aptIface("org.debian.apt", "/org/debian/apt", "org.debian.apt", bus);
    
    QString trans = aptIface.call("UpgradeSystem", true).arguments().value(0).toString();
    if (trans.isEmpty()) return false;

    QDBusInterface( "org.debian.apt", trans, "org.debian.apt.transaction", bus ).call("Simulate");

    QDBusMessage reply = QDBusInterface("org.debian.apt", trans, "org.freedesktop.DBus.Properties", bus)
                         .call("Get", "org.debian.apt.transaction", "Dependencies");

    const QDBusArgument arg = reply.arguments().value(0).value<QDBusVariant>().variant().value<QDBusArgument>();
    return !arg.atEnd();
}

bool LliurexUpIndicatorUtils::checkRemote() {
    
    QProcess ps;
    ps.start("ps", QStringList() << "-e" << "-o" << "tty=" << "-o" << "args=");
    ps.waitForFinished(1000);
    QString psOut = QString::fromUtf8(ps.readAllStandardOutput());

    QStringList remoteTtys;
    QStringList lines = psOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        if (line.contains("lliurex-upgrade")) {
            QString tty = line.section(' ', 0, 0).trimmed();
            if (tty == "?") return true;
            remoteTtys << tty;
        }
    }

    if (remoteTtys.isEmpty()) return false;

    QProcess who;
    who.start(QStringLiteral("who"), QStringList());
    who.waitForFinished(1000);
    QString whoOut = QString::fromUtf8(who.readAllStandardOutput());

    for (const QString &tty : remoteTtys) {
        QStringList whoLines = whoOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const QString &wLine : whoLines) {
            if (wLine.contains(tty) && !wLine.contains(":0")) {
                return true;
            }
        }
    }

    return false;
}

bool LliurexUpIndicatorUtils::isCacheUpdated(){

    QDateTime currentTime=QDateTime::currentDateTime();
    QDateTime lastModification=QFileInfo(PKGCACHE).lastModified();    

    qint64 millisecondsDiff = lastModification.msecsTo(currentTime);

    if (millisecondsDiff<APT_FRECUENCY){
        cacheUpdated=true;
    }
    return cacheUpdated;

}

bool LliurexUpIndicatorUtils::isConnectionWithServer(){

    try{
        n4d::Client client;
        client=n4d::Client("https://server:9779");
        variant::Variant test=client.call("MirrorManager","is_alive");
        return true;
               
    }catch(...){
        return false;
    }
     
}

bool LliurexUpIndicatorUtils::canStopAutoUpdate(){

    try{
        n4d::Client localClient;
        localClient=n4d::Client("https://localhost:9779");
        variant::Variant result=localClient.call("LliurexUpManager","can_cancel_auto_upgrade");
        return result;
    }catch(...){
        return false;
    }

}

bool LliurexUpIndicatorUtils::isAutoUpdateReady(){

    if (AUTO_UPDATE_TOKEN.exists()){
        return true;
    }else{
        return false;
    }
   
}

bool LliurexUpIndicatorUtils::isAutoUpdateRun(){
    
    if (AUTO_UPDATE_RUN_TOKEN.exists()){
        return true;
    }else{
        return false;
    }
}

QString LliurexUpIndicatorUtils::getAutoUpdateTime(){

    QString result;
    if (LliurexUpIndicatorUtils::isAutoUpdateReady()){
        if (AUTO_UPDATE_TOKEN.open(QIODevice::ReadOnly)){
            QTextStream content(&AUTO_UPDATE_TOKEN);
            result=content.readLine();
            AUTO_UPDATE_TOKEN.close();
        }

    }
    return result;
}

void LliurexUpIndicatorUtils::stopAutoUpdate(){

    try{
        if (!AUTO_UPDATE_RUN_TOKEN.exists()){
            n4d::Client localClient;
            localClient=n4d::Client("https://localhost:9779");
            localClient.call("LliurexUpManager","stop_auto_update_service");
        }
    }catch(...){
        
    }
    
}
