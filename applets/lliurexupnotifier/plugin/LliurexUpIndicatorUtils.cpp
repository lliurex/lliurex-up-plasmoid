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
#include "LliurexUpIndicatorServiceSubscriber.h"

#include <QFile>
#include <QDateTime>
#include <QFileInfo>
#include <QDebug>
#include <QTextStream>
#include <QTimer>
#include <QPointer>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusVariant>
#include <QtConcurrent>
#include <QThreadPool>

#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>

LliurexUpIndicatorUtils::LliurexUpIndicatorUtils()
    : QObject(nullptr)
    , m_service(new LliurexUpIndicatorServiceSubscriber(this))
    , m_isStarting(false)
    , m_isStarted(false)
    , m_hideWidget(false)
    , m_updatingCache(false)
    , m_isCheckingUpdates(false)
    , m_isCreatingSubscription(false)
    , m_isSubscribed(false)
    , cacheUpdated(true)
       
{
    user=qgetenv("USER");
    PKGCACHE.setFileName("/var/cache/apt/pkgcache.bin");
    connect(m_service,&LliurexUpIndicatorServiceSubscriber::createSubscriptionFinished,this,&LliurexUpIndicatorUtils::handleCreateSubscriptionFinished);
    connect(m_service,&LliurexUpIndicatorServiceSubscriber::serviceStateChanged,this,&LliurexUpIndicatorUtils::handleManageService);

}

void LliurexUpIndicatorUtils::startWidget(){

    if (m_isStarted){
        emit startWidgetFinished(m_hideWidget);
        return;
    }

    if (m_isStarting){
        return;
    }

    m_isStarting=true;

    QPointer<LliurexUpIndicatorUtils>safeThis(this);

    QtConcurrent::run(QThreadPool::globalInstance(),[safeThis]() {

        if (!safeThis){
            return;
        }

        bool hideWidget=true;
        QStringList userGroups=safeThis->getUserGroups();

        if (userGroups.length()>0){
            hideWidget=false;
        }
        if (safeThis){
            safeThis->m_isStarting=false;
            safeThis->m_isStarted=true;
            safeThis->m_hideWidget=hideWidget;
            emit safeThis->startWidgetFinished(hideWidget);
        }
    });
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
            if ((strcmp(gr->gr_name,"adm")==0)||(strcmp(gr->gr_name,"admins")==0)||(strcmp(gr->gr_name,"teachers")==0) || (strcmp(gr->gr_name,"NoDocente")==0)){
                userGroups.append(gr->gr_name);
            }else if (strcmp(gr->gr_name,"students")==0){
                userGroups.append(gr->gr_name);
                isStudent=true;
            } 
        }  
    }
    return userGroups;
}

void LliurexUpIndicatorUtils::createSubscription(){

    if (m_isSubscribed){
        return;
    }

    if (m_isCreatingSubscription){
        return;
    }

    m_isCreatingSubscription=true;
    m_service->initSubscription();
}

void LliurexUpIndicatorUtils::handleCreateSubscriptionFinished(bool startOk){

    m_isCreatingSubscription=false;
    m_isSubscribed=true;

}

void LliurexUpIndicatorUtils::handleManageService(bool autoUpgradeRun){

    emit unitStateChange(autoUpgradeRun);
}

void LliurexUpIndicatorUtils::runUpdateCache() {

    if (m_updatingCache) {
        return;
    }

    if (cacheUpdated) {
        cacheUpdated=false;
        checkUpdates();
        return;
    }
    
    m_updatingCache = true;

    QDBusConnection bus = QDBusConnection::systemBus();

    if (!bus.isConnected()) {
        m_updatingCache = false;
        emit updatesFound(false);
        return;
    }

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
        QDBusPendingReply<QString> reply = *self;
        self->deleteLater();
        aptIface->deleteLater();

        if (reply.isError()) {
            qDebug()<<"[LLIUREX-UP-INDICATOR]: runUpdateCache reply error: "<<reply.error().message();
            m_updatingCache = false;
            emit updatesFound(false);
            return;
        }

        QString path = reply.value();

        if (path.isEmpty()) {
            qDebug()<<"[LLIUREX-UP-INDICATOR]: runUpdateCache empty path"<<path;
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
            QDBusMessage msg=QDBusMessage::createMethodCall(
                QStringLiteral("org.debian.apt"),
                path,
                QStringLiteral("org.debian.apt.transaction"),
                QStringLiteral("Run")
            );

            QDBusPendingCall runCall=bus.asyncCall(msg);
            QDBusPendingCallWatcher *runWatcher = new QDBusPendingCallWatcher(runCall, this);

            connect(runWatcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *runSelf) {
                QDBusPendingReply<> runReply = *runSelf;
                runSelf->deleteLater();

                if (runReply.isError()){
                    qDebug()<<"[LLIUREX-UP-INDICATOR]: runUpdateCache error execution call:"<<runReply.error().message();
                    m_updatingCache=false;
                    emit updatesFound(false);

                    if (m_transIface){
                        m_transIface->deleteLater();
                        m_transIface=nullptr;
                    }
                }
            });

        }else{
            m_updatingCache = false;
            m_transIface->deleteLater();
            m_transIface = nullptr;
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

    if (m_isCheckingUpdates){
        return;
    }

    m_isCheckingUpdates=true;

    QDBusConnection bus = QDBusConnection::systemBus();

    if (!bus.isConnected()) {
        m_isCheckingUpdates=false;
        emit updatesFound(false);
        return;
    }

    auto *aptIface = new QDBusInterface("org.debian.apt", "/org/debian/apt", "org.debian.apt", bus, this);
    
    if (!aptIface->isValid()){
        m_isCheckingUpdates=false;
        aptIface->deleteLater();
        emit updatesFound(false);
        return;
    }

    QDBusPendingCall call = aptIface->asyncCall("UpgradeSystem", true);
    auto *watcher = new QDBusPendingCallWatcher(call, aptIface);

    QPointer<LliurexUpIndicatorUtils> safeThis(this);
    QPointer<QDBusInterface> safeIface(aptIface);
    QPointer<QDBusPendingCallWatcher>safeWatcher(watcher);

    auto finished = QSharedPointer<bool>::create(false);

    QTimer::singleShot(30000, this, [safeThis, safeIface,safeWatcher,finished,call,bus]() {

        if (!safeThis || *finished) {
            return;
        }

        *finished=true;

        if (call.isFinished()) {
            QDBusPendingReply<QString> reply = call;
            if (!reply.isError() && !reply.value().isEmpty()) {
                QDBusInterface cancelIface("org.debian.apt", reply.value(), "org.debian.apt.transaction", bus);
                cancelIface.call(QDBus::NoBlock, "Cancel");
            }
        }
        
        if (safeWatcher){
            safeWatcher->deleteLater();
        }
        
        if (safeIface) {
            safeIface->deleteLater(); 
        }
      
        safeThis->m_isCheckingUpdates=false;
        emit safeThis->updatesFound(false);
    
    });

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [safeThis, safeIface, safeWatcher,finished](QDBusPendingCallWatcher *self) {
        
        if (!safeThis || *finished){
             return;
        }

        *finished=true;

        if (safeWatcher){
            safeWatcher->deleteLater();
        }

        if (safeIface) {
            safeIface->deleteLater();
    
        }

        QDBusPendingReply<QString> reply = *self;

        if (reply.isError()) {
            qDebug()<<"[LLIUREX-UP-INDICATOR]: checkUpdates reply error:"<<reply.error().message();
            safeThis->m_isCheckingUpdates=false;
            emit safeThis->updatesFound(false);
            return;
        }
        
        QString transPath = reply.value(); 

        if (!transPath.isEmpty()) {
            safeThis->runSimulation(transPath);
        }else{
            qDebug()<<"[LLIUREX-UP-INDICATOR]: checkUpdates empty path:"<<transPath;
            safeThis->m_isCheckingUpdates=false;
            emit safeThis->updatesFound(false);
        }
    });
}

void LliurexUpIndicatorUtils::runSimulation(const QString &transPath) {

    QDBusConnection bus = QDBusConnection::systemBus();

    if (!bus.isConnected()) {
        m_isCheckingUpdates=false;
        emit updatesFound(false);
        return;
    }

    auto *transIface = new QDBusInterface("org.debian.apt", transPath, "org.debian.apt.transaction", bus, this);
    
    if (!transIface->isValid()){
        m_isCheckingUpdates=false;
        transIface->deleteLater();
        emit updatesFound(false);
        return;
    }

    QDBusPendingCall simCall = transIface->asyncCall(QStringLiteral("Simulate"));
    auto *simWatcher = new QDBusPendingCallWatcher(simCall, this);

    QPointer<LliurexUpIndicatorUtils> safeThis(this);
    QPointer<QDBusInterface> safeTrans(transIface);
    QPointer<QDBusPendingCallWatcher>safeWatcher(simWatcher);

    auto finished = QSharedPointer<bool>::create(false);

    QTimer::singleShot(60000, this, [safeThis, safeTrans, safeWatcher,finished ]() {

        if (!safeThis || *finished) {
            return;
        }

        *finished = true;

        if (safeTrans) {
            safeTrans->call(QDBus::NoBlock, "Cancel");
            safeTrans->deleteLater();
        }

        if (safeWatcher){
            safeWatcher->deleteLater();
        }
        
        safeThis->m_isCheckingUpdates=false;
        safeThis->updatesFound(false);
    });

    connect(simWatcher, &QDBusPendingCallWatcher::finished, this, [safeThis, safeTrans, safeWatcher, finished]() {

        if (!safeThis || *finished) {
             return;
        }
        
        *finished = true;

        QString currentPath= safeTrans ? safeTrans->path() : QString();

        if (safeWatcher){
            safeWatcher->deleteLater();
        }

        if (safeTrans){
            safeTrans->deleteLater();
        }

        if (currentPath.isEmpty()){
            safeThis->m_isCheckingUpdates=false;
            emit safeThis->updatesFound(false);
            return;
        }

        QDBusMessage msg = QDBusInterface("org.debian.apt", safeTrans->path(), 
                                         "org.freedesktop.DBus.Properties", 
                                         QDBusConnection::systemBus())
                           .call("Get", "org.debian.apt.transaction", "Dependencies");

        safeTrans->deleteLater();

        if (msg.type() == QDBusMessage::ErrorMessage) {
            qDebug()<<"[LLIUREX-UP-INDICATOR]: runSimulation error :"<<msg.errorMessage();
            safeThis->m_isCheckingUpdates=false;
            emit safeThis->updatesFound(false);
            return;
        }

        if (msg.arguments().isEmpty()){
            safeThis->m_isCheckingUpdates=false;
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
        
        safeThis->m_isCheckingUpdates=false;
        emit safeThis->updatesFound(thereAreUpdates);
    });
}

bool LliurexUpIndicatorUtils::checkRemote() {

    QProcess pgrep;
    pgrep.start("pgrep", QStringList() << "-f" << "lliurex-upgrade");
    pgrep.waitForFinished(500);
    QString pid = QString::fromUtf8(pgrep.readAllStandardOutput()).trimmed().split("\n").first();

    if (pid.isEmpty()) return false;

    QString currentPid = pid;

    for (int i = 0; i < 10; ++i) {
        QProcess ps;
        ps.start("ps", QStringList() << "-p" << currentPid << "-o" << "comm=,tty=,ppid=");
        ps.waitForFinished(500);

        QString output = QString::fromUtf8(ps.readAllStandardOutput()).trimmed();
        if (output.isEmpty()) break;

        QStringList parts = output.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

        if (parts.size() < 3) break;

        QString name = parts[0];
        QString tty  = parts[1];
        QString ppid = parts[2];

        if (name.contains("sshd")){
            return true;
        }

        if (name.contains("konsole")) {
            return false;
        }

        if (tty == "?" || tty.isEmpty()) {
            if (i==0){
                return true;
            }
        }

        if (ppid == "0" || ppid == "1" || currentPid == ppid) break;
        currentPid = ppid;
    }

    return false;
}

bool LliurexUpIndicatorUtils::isCacheUpdated(){

    QDateTime currentTime=QDateTime::currentDateTime();

    if (lastCheckTime.isValid() && lastCheckTime.msecsTo(currentTime)<1000){
        return cacheUpdated;
    }
 
    QFileInfo pkgCacheInfo(PKGCACHE);

    if (!pkgCacheInfo.exists()){
        cacheUpdated=false;
        return cacheUpdated;
    }

    lastCheckTime=currentTime;

    QDateTime lastModification=pkgCacheInfo.lastModified();    

    qint64 millisecondsDiff = lastModification.msecsTo(currentTime);

    if (millisecondsDiff<APT_FRECUENCY){
        cacheUpdated=true;
    }else{
        cacheUpdated=false;
    }

    return cacheUpdated;
}
