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

void LliurexUpIndicatorUtils::runUpdateCache() {

    if (m_updatingCache || cacheUpdated) {
        cacheUpdated=false;
        checkUpdates();
        return;
    }

    QDBusConnection bus = QDBusConnection::systemBus();

    if (!bus.isConnected()) {
        emit updatesFound(false);
        return;
    }

    m_updatingCache = true;
    QDBusInterface *aptIface = new QDBusInterface("org.debian.apt", "/org/debian/apt", "org.debian.apt", bus, this);

    if (!aptIface->isValid()) {
        m_updatingCache = false;
        aptIface->deleteLater();
        emit updatesFound(false); 
        return;
    }

    QDBusPendingCall pcall = aptIface->asyncCall(QStringLiteral("UpdateCache"));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pcall, this);
    
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, bus, aptIface](QDBusPendingCallWatcher *self) {
        QDBusPendingReply<QDBusObjectPath> reply = *self;
        self->deleteLater();
        aptIface->deleteLater();

        if (reply.isError()) {
            m_updatingCache = false;
            emit updatesFound(false);
            return;
        }

        QString path = reply.value().path();
        if (path.isEmpty()) {
            m_updatingCache = false;
            emit updatesFound(false);
            return;
        }

        if (m_transIface) {
            m_transIface->disconnect();
            m_transIface->deleteLater();
            m_transIface = nullptr;
        }

        m_transIface = new QDBusInterface("org.debian.apt", path, "org.debian.apt.transaction", bus, this);

        if (m_transIface->isValid()) {
            connect(m_transIface, SIGNAL(Finished(QString)), this, SLOT(onCacheFinished()));
            m_transIface->asyncCall(QStringLiteral("Run"));
        }else{
            m_updatingCache = false;
            emit updatesFound(false);
        }
    });
}  

void LliurexUpIndicatorUtils::onCacheFinished(){

    cacheUpdated=false;
    m_updatingCache = false;

    if (m_transIface) {
        m_transIface->disconnect();
        m_transIface->deleteLater();
        m_transIface =nullptr;
    }

    checkUpdates();
}

void LliurexUpIndicatorUtils::checkUpdates() {

    QDBusConnection bus = QDBusConnection::systemBus();

    if (!bus.isConnected()) {
        emit updatesFound(false);
        return;
    }

    auto *aptIface = new QDBusInterface("org.debian.apt", "/org/debian/apt", "org.debian.apt", bus, this);
    QDBusPendingCall call = aptIface->asyncCall("UpgradeSystem", true);
    auto *watcher = new QDBusPendingCallWatcher(call, aptIface);

    QPointer<LliurexUpIndicatorUtils> safeThis(this);
    QPointer<QDBusInterface> safeIface(aptIface);
    QSharedPointer<bool> finished = QSharedPointer<bool>::create(false);

    QTimer::singleShot(30000, this, [safeThis, safeIface,finished,call,bus]() {

        if (*finished) return;

        *finished=true;

        if (call.isFinished()) {
            QDBusPendingReply<QString> reply = call;
            if (!reply.isError() && !reply.value().isEmpty()) {
                QDBusInterface cancelIface("org.debian.apt", reply.value(), "org.debian.apt.transaction", bus);
                cancelIface.call(QDBus::NoBlock, "Cancel");
            }
        }

        if (safeIface) {
            safeIface->deleteLater(); 
            if (safeThis) emit safeThis->updatesFound(false);
        }
    });

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [safeThis, safeIface, finished](QDBusPendingCallWatcher *self) {

        if (*finished){
            safeIface->deleteLater();
            return;
        }
        *finished=true;

        if (!safeThis || !safeIface) {
            return;
        }   

        QDBusPendingReply<QString> reply = *self;

        safeIface->deleteLater();

        if (reply.isError()) {
            emit safeThis->updatesFound(false);
        }else{
            QString transPath = reply.value(); 

            if (!transPath.isEmpty()) {
                safeThis->runSimulation(transPath);
            }else{
                emit safeThis->updatesFound(false);
            }
        }
    });
}

void LliurexUpIndicatorUtils::runSimulation(const QString &transPath) {

    QDBusConnection bus = QDBusConnection::systemBus();

    if (!bus.isConnected()) {
        emit updatesFound(false);
        return;
    }

    auto *transIface = new QDBusInterface("org.debian.apt", transPath, "org.debian.apt.transaction", bus, this);
    QDBusPendingCall simCall = transIface->asyncCall(QStringLiteral("Simulate"));
    auto *simWatcher = new QDBusPendingCallWatcher(simCall, transIface);

    QPointer<LliurexUpIndicatorUtils> safeThis(this);
    QPointer<QDBusInterface> safeTrans(transIface);
    QSharedPointer<bool> finished = QSharedPointer<bool>::create(false);

    QTimer::singleShot(60000, this, [safeThis, safeTrans, finished, transPath, bus]() {
        if (*finished) return;
        *finished = true;

        if (safeTrans) {
            safeTrans->call(QDBus::NoBlock, "Cancel");
            safeTrans->deleteLater();
        }
        if (safeThis) emit safeThis->updatesFound(false);
    });

    connect(simWatcher, &QDBusPendingCallWatcher::finished, this, [safeThis, safeTrans, finished](QDBusPendingCallWatcher *self) {
        
        if (*finished) {
            self->deleteLater();
            return;
        }

        *finished = true;

        if (!safeThis || !safeTrans) return;
        QDBusMessage msg = QDBusInterface("org.debian.apt", safeTrans->path(), 
                                         "org.freedesktop.DBus.Properties", 
                                         QDBusConnection::systemBus())
                           .call("Get", "org.debian.apt.transaction", "Dependencies");

        safeTrans->deleteLater();

        if (msg.type() == QDBusMessage::ErrorMessage) {
            emit safeThis->updatesFound(false);
            return;
        }

        QVariant var = msg.arguments().at(0).value<QDBusVariant>().variant();
        const QDBusArgument arg = var.value<QDBusArgument>();

        QList<QStringList> pkgLists;
        arg >> pkgLists; 
        bool thereAreUpdates = false;

        if (pkgLists.size() >= 5) {
            // Índex 4: Upgrades
            // Índex 0: Installs
            thereAreUpdates = !pkgLists.at(4).isEmpty() || !pkgLists.at(0).isEmpty();
        }

        emit safeThis->updatesFound(thereAreUpdates);
    });
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