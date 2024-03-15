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
#include "LliurexUpIndicator.h"
#include "LliurexUpIndicatorUtils.h"

#include <KLocalizedString>
#include <KFormat>
#include <KNotification>
#include <KIO/CommandLauncherJob>
#include <QTimer>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>
#include <QThread>
#include <QFileSystemWatcher>


LliurexUpIndicator::LliurexUpIndicator(QObject *parent)
    : QObject(parent)
    ,m_timer(new QTimer(this))
    ,m_timer_run(new QTimer(this))
    ,m_timer_cache(new QTimer(this))
    ,m_utils(new LliurexUpIndicatorUtils(this))
    
{
    
    TARGET_FILE.setFileName("/var/run/lliurexUp.lock");
    DISABLE_WIDGET_TOKEN.setFileName("/etc/lliurex-up-indicator/disableIndicator.token");
   
    plasmoidMode();

    connect(m_timer, &QTimer::timeout, this, &LliurexUpIndicator::worker);
    m_timer->start(1200000);
    initWatcher();
    worker();
}    


void LliurexUpIndicator::plasmoidMode(){

    bool isClient=false;
    bool isDesktop=false;
    bool hideIndicator=false;

    QStringList flavours=m_utils->getFlavours();
    QStringList userGroups=m_utils->getUserGroups();

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
                            hideIndicator=true;
                            break;
                        }
                    }
                }
            }
        }
    }else{
        hideIndicator=true;
    }

    if (isClient){
        if (isDesktop){
            if (m_utils->isConnectionWithServer()){
                hideIndicator=true;
            }
        }else{
            hideIndicator=true;
        }
    }

    if (!hideIndicator){
        updatedInfo=true;
    }
 
}

void LliurexUpIndicator::initWatcher(){

    QDir TARGET_DIR(refPath);

    if (TARGET_DIR.exists()){
        isLliurexUpRunning();
    }

    watcher=new QFileSystemWatcher(this);
    connect(watcher,SIGNAL(directoryChanged(QString)),this,SLOT(isLliurexUpRunning()));
    watcher->addPath(refPath);

}

void LliurexUpIndicator::isLliurexUpRunning(){

    if (!isWorking){
        if (LliurexUpIndicator::TARGET_FILE.exists()) {
            isAlive();
        }
    }
}

void LliurexUpIndicator::worker(){

    if (!isWorking){
        if (LliurexUpIndicator::TARGET_FILE.exists() ) {
            isAlive();
        }else{
            if (!LliurexUpIndicator::DISABLE_WIDGET_TOKEN.exists()) {
                if (updatedInfo){
                    if (!remoteUpdateInfo){
                        lastUpdate=lastUpdate+1200;
                        if (lastUpdate==FREQUENCY){
                            lastUpdate=0;
                            updateCache();

                        }else{
                            if (m_utils->isCacheUpdated()){
                                lastUpdate=0;
                                updateCache();
                            }
                        }
                    }
                }
            }
        }

    }

}    

void LliurexUpIndicator::updateCache(){

    isWorking=true;

    adbus=new AsyncDbus(this);
    QObject::connect(adbus,SIGNAL(message(bool)),this,SLOT(dbusDone(bool)));
    adbus->start();

}

bool LliurexUpIndicator::runUpdateCache(){

    return m_utils->runUpdateCache();

}

void LliurexUpIndicator::dbusDone(bool result){

    isWorking=false;
        
    adbus->exit(0);
    if (adbus->wait()){
        delete adbus;
    }

    if (result){
        changeTryIconState(0);
    }

}    

void LliurexUpIndicator::isAlive(){

    isWorking=true;
    remoteUpdateInfo=true;

    if (m_utils->checkRemote()){
        changeTryIconState(2);
    }else{
        changeTryIconState(1);
    }

    connect(m_timer_run, &QTimer::timeout, this, &LliurexUpIndicator::checkLlxUp);
    m_timer_run->start(5000);
    checkLlxUp();


}

void LliurexUpIndicator::checkLlxUp(){

    if (!LliurexUpIndicator::TARGET_FILE.exists()){
        m_timer_run->stop();
        isWorking=false;
        remoteUpdateInfo=false;
        changeTryIconState(1);
          
    } 

}

LliurexUpIndicator::TrayStatus LliurexUpIndicator::status() const
{
    return m_status;
}

void LliurexUpIndicator::changeTryIconState(int state){

    const QString tooltip(i18n("Lliurex-Up"));
    QString notificationTitle;
    QString notificationBody;
    QString notificationIcon;

    if (state==0){
        const QString subtooltip(i18n("There are new packages ready to be updated or installed"));
        setStatus(ActiveStatus);
        if (m_utils->isAutoUpgradeReady()){
            setIconName("lliurexupnotifier-autoupdate");
            QString timeToUpdate=m_utils->getAutoUpgradeTime();
            notificationBody=i18n("The system will be update automatically at")+" "+timeToUpdate;
            notificationIcon="lliurexupnotifier-autoupdate";
        }else{
            setIconName("lliurexupnotifier");
            notificationIcon="lliurexupnotifier";
        }
        setToolTip(tooltip);
        setSubToolTip(subtooltip+"\n"+notificationBody);
        m_updatesAvailableNotification = KNotification::event(QStringLiteral("Update"), subtooltip, notificationBody, notificationIcon, nullptr, KNotification::CloseOnTimeout , QStringLiteral("llxupnotifier"));
        const QString name = i18n("Update now");
        m_updatesAvailableNotification->setDefaultAction(name);
        m_updatesAvailableNotification->setActions({name});
        connect(m_updatesAvailableNotification, QOverload<unsigned int>::of(&KNotification::activated), this, &LliurexUpIndicator::launch_llxup);

    }else if (state==2){
        setStatus(ActiveStatus);
        notificationTitle=i18n("Lliurex-Up is being run remotely or automatically");
        notificationBody=i18n("Do not turn-off or restart your computer");
        const QString subtooltip=notificationTitle+"\n"+notificationBody;
        setToolTip(tooltip);
        setSubToolTip(subtooltip);
        setIconName("lliurexupnotifier-running");
        notificationIcon="lliurexupnotifier-running";
        m_remoteUpdateNotification = KNotification::event(QStringLiteral("remoteUpdate"), notificationTitle,notificationBody, notificationIcon, nullptr, KNotification::Persistent , QStringLiteral("llxupnotifier"));

    }else{
        if (m_remoteUpdateNotification) { m_remoteUpdateNotification->close(); }
        setStatus(PassiveStatus);
    }
    
}

void LliurexUpIndicator::launch_llxup()
{
    if (m_status==0){
        KIO::CommandLauncherJob *job = nullptr;
        QString cmd="lliurex-up-desktop-launcher.py";
        job = new KIO::CommandLauncherJob(cmd);
        job->start();
        if (m_updatesAvailableNotification) { m_updatesAvailableNotification->close(); }
    }    
   
}

void LliurexUpIndicator::setStatus(LliurexUpIndicator::TrayStatus status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
    }
}

QString LliurexUpIndicator::iconName() const
{
    return m_iconName;
}

void LliurexUpIndicator::setIconName(const QString &name)
{
    if (m_iconName != name) {
        m_iconName = name;
        emit iconNameChanged();
    }
}

QString LliurexUpIndicator::toolTip() const
{
    return m_toolTip;
}

void LliurexUpIndicator::setToolTip(const QString &toolTip)
{
    if (m_toolTip != toolTip) {
        m_toolTip = toolTip;
        emit toolTipChanged();
    }
}

QString LliurexUpIndicator::subToolTip() const
{
    return m_subToolTip;
}

void LliurexUpIndicator::setSubToolTip(const QString &subToolTip)
{
    if (m_subToolTip != subToolTip) {
        m_subToolTip = subToolTip;
        emit subToolTipChanged();
    }
}
