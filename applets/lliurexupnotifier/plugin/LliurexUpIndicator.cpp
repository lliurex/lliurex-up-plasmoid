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
#include <QDate>
#include <QTime>


LliurexUpIndicator::LliurexUpIndicator(QObject *parent)
    : QObject(parent)
    ,m_timer(new QTimer(this))
    ,m_timer_run(new QTimer(this))
    ,m_timer_cache(new QTimer(this))
    ,m_utils(new LliurexUpIndicatorUtils(this))
    
{
    
    TARGET_FILE.setFileName("/var/run/lliurexUp.lock");
    DISABLE_WIDGET_TOKEN.setFileName("/etc/lliurex-up-indicator/disableIndicator.token");
    
    m_utils->cleanCache();

    QString initTitle=i18n("No updates availables");
    setSubToolTip(initTitle);
    setCanLaunchLlxUp(false);
    setCanStopAutoUpdate(false);
    plasmoidMode();

    connect(m_timer, &QTimer::timeout, this, &LliurexUpIndicator::worker);
    m_timer->start(1200000);
    initWatcher();
    worker();
}    


void LliurexUpIndicator::plasmoidMode(){

    QStringList userGroups=m_utils->getUserGroups();
    if (userGroups.size()>0){
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
        }else{
            if (m_utils->isAutoUpdateReady()){
                if (thereAreUpdates){
                    if (!autoUpdatesDisplayed){
                        changeTryIconState(0,true);
                    }else{
                        changeTryIconState(0,false);
                    }
                }
                
            }else{
                if (autoUpdatesDisplayed){
                    hideAutoUpdate();
                }
            }
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
        thereAreUpdates=result;
        changeTryIconState(0,true);
    }

}    

void LliurexUpIndicator::isAlive(){

    isWorking=true;
    remoteUpdateInfo=true;

    if (m_utils->checkRemote()){
        changeTryIconState(2,true);
    }else{
        changeTryIconState(1,false);
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
        if (m_utils->isAutoUpdateReady()){
            if (thereAreUpdates){
                if (!autoUpdatesDisplayed){
                    changeTryIconState(0,true);
                }else{
                    changeTryIconState(0,false);
                }
            }else{
                changeTryIconState(1,false); 
            }
        }else{
            changeTryIconState(1,false);
        }
          
    } 

}

LliurexUpIndicator::TrayStatus LliurexUpIndicator::status() const
{
    return m_status;
}

void LliurexUpIndicator::changeTryIconState(int state,bool showNotification=true){

    const QString tooltip(i18n("Lliurex-Up"));

    QString notificationTitle;
    QString notificationBody;
    QString notificationIcon;
    QString pauseInfo;
    bool showStopOption=false;

    if (state==0){
        const QString subtooltip(i18n("There are new packages ready to be updated or installed"));
    
        if (m_utils->isAutoUpdateReady()){
            if (m_utils->isStudent){
                setCanLaunchLlxUp(false);
            }else{
                setCanLaunchLlxUp(true);
            }
            setStatus(ActiveStatus);
            setIconName("lliurexupnotifier-autoupdate");
            if (m_utils->canStopAutoUpdate()){
                if (!m_utils->isAutoUpdateRun()){
                    setCanStopAutoUpdate(true);
                    showStopOption=true;
                }else{
                    setCanStopAutoUpdate(false);
                }
            }else{
                setCanStopAutoUpdate(false);
            }
            autoUpdatesDisplayed=true;
            QString timeToUpdate=m_utils->getAutoUpdateTime();
            notificationBody=i18n("The system will be update automatically at")+" "+timeToUpdate;
            notificationIcon="lliurexupnotifier-autoupdate";
            setToolTip(tooltip);
            setSubToolTip(subtooltip+"\n"+notificationBody);
            if (showNotification){
                m_updatesAvailableNotification = new KNotification(QStringLiteral("Update"), KNotification::CloseOnTimeout,this);
                m_updatesAvailableNotification->setComponentName(QStringLiteral("llxupnotifier"));
                m_updatesAvailableNotification->setTitle(subtooltip);
                m_updatesAvailableNotification->setText(notificationBody);
                m_updatesAvailableNotification->setIconName(notificationIcon);
                const QString name = i18n("Wait until tomorrow");
                if (showStopOption){
                    auto cancelUpdateAction=m_updatesAvailableNotification->addAction(name);
                    connect(cancelUpdateAction,&KNotificationAction::activated,this,&LliurexUpIndicator::cancelAutoUpdate);
                    m_updatesAvailableNotification->sendEvent();
                }
            }
        }else{
            if (!m_utils->isStudent){
                setStatus(ActiveStatus);
                setIconName("lliurexupnotifier");
                setCanLaunchLlxUp(true);
                setCanStopAutoUpdate(false);
                notificationIcon="lliurexupnotifier";
                QString tmpDate=m_utils->getAutoUpdatePause();
                
                if (tmpDate!=""){
                    pauseInfo=i18n("Automatic updates paused until ")+tmpDate;
                    setSubToolTip(subtooltip+"\n"+pauseInfo);
                }else{
                    setSubToolTip(subtooltip);
                }
                
                setToolTip(tooltip);

                if ((showNotification)&&(rememberUpdate)){
                    m_updatesAvailableNotification = new KNotification(QStringLiteral("Update"), KNotification::CloseOnTimeout,this);
                    m_updatesAvailableNotification->setComponentName(QStringLiteral("llxupnotifier"));
                    m_updatesAvailableNotification->setTitle(subtooltip);
                    m_updatesAvailableNotification->setText(notificationBody);
                    m_updatesAvailableNotification->setIconName(notificationIcon);
                    const QString name = i18n("Update now");
                    auto updateAction=m_updatesAvailableNotification->addAction(name);
                    connect(updateAction,&KNotificationAction::activated,this,&LliurexUpIndicator::launchLlxup);
                     m_updatesAvailableNotification->sendEvent();
                }
            }
        }

    }else if (state==2){
        setStatus(ActiveStatus);
        setCanLaunchLlxUp(false);
        setCanStopAutoUpdate(false);
        notificationTitle=i18n("Lliurex-Up is being run remotely or automatically");
        notificationBody=i18n("Do not turn-off or restart your computer");
        const QString subtooltip=notificationTitle+"\n"+notificationBody;
        setToolTip(tooltip);
        setSubToolTip(subtooltip);
        setIconName("lliurexupnotifier-running");
        notificationIcon="lliurexupnotifier-running";
        m_remoteUpdateNotification = new KNotification(QStringLiteral("remoteUpdate"), KNotification::Persistent,this);
        m_remoteUpdateNotification->setComponentName(QStringLiteral("llxupnotifier"));
        m_remoteUpdateNotification->setTitle(notificationTitle);
        m_remoteUpdateNotification->setText(notificationBody);
        m_remoteUpdateNotification->setIconName(notificationIcon);
        m_remoteUpdateNotification->sendEvent();

    }else {
        setStatus(PassiveStatus);
        setCanLaunchLlxUp(false);
        setCanStopAutoUpdate(false);
        QDate currentDate=QDate::currentDate();
        QString lastDay=currentDate.toString(Qt::ISODate);
        QTime currentTime=QTime::currentTime();
        QString lastTime=currentTime.toString(Qt::ISODate);
        QString endTitle=i18n("Last execution: ")+lastDay+" "+lastTime;
        setSubToolTip(endTitle);
        if (m_remoteUpdateNotification) { m_remoteUpdateNotification->close(); }
        setStatus(PassiveStatus);
    }
    
}

void LliurexUpIndicator::launchLlxup()
{
    if (m_status==0){
        KIO::CommandLauncherJob *job = nullptr;
        QString cmd="lliurex-up-desktop-launcher.py";
        job = new KIO::CommandLauncherJob(cmd);
        job->start();
        if (m_updatesAvailableNotification) { m_updatesAvailableNotification->close(); }
    }    
   
}

void LliurexUpIndicator::cancelAutoUpdate()
{

    m_utils->stopAutoUpdate();
    hideAutoUpdate();
    
}

void LliurexUpIndicator::hideAutoUpdate(){

    if (m_updatesAvailableNotification) { m_updatesAvailableNotification->close(); }
        if (m_utils->isStudent){
            changeTryIconState(1);
        }else{
            rememberUpdate=false;    
            changeTryIconState(0,false);
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

bool LliurexUpIndicator::canLaunchLlxUp()
{
    return m_canLaunchLlxUp;

} 

void LliurexUpIndicator::setCanLaunchLlxUp(bool canLaunchLlxUp){

    if (m_canLaunchLlxUp != canLaunchLlxUp){
        m_canLaunchLlxUp = canLaunchLlxUp;
        emit canLaunchLlxUpChanged();
    }
}

bool LliurexUpIndicator::canStopAutoUpdate()
{
    return m_canStopAutoUpdate;

}

void LliurexUpIndicator::setCanStopAutoUpdate(bool canStopAutoUpdate){

    if (m_canStopAutoUpdate != canStopAutoUpdate){
        m_canStopAutoUpdate = canStopAutoUpdate;
        emit canStopAutoUpdateChanged();
    }
}
