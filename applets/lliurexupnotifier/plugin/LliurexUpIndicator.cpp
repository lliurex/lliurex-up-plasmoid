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
#include <KNotification>
#include <KIO/CommandLauncherJob>


#include <QTimer>
#include <QDebug>
#include <QFileSystemWatcher>
#include <QDateTime>

#include <QPointer>

LliurexUpIndicator::LliurexUpIndicator(QObject *parent)
    : QObject(parent)
    ,m_timer(new QTimer(this))
    ,m_watcher_timer(new QTimer(this))
    ,m_utils(&LliurexUpIndicatorUtils::instance())

{
    TARGET_FILE.setFileName(m_utils->refPath+"/lliurexUp.lock");
    DISABLE_WIDGET_TOKEN.setFileName("/etc/lliurex-up-indicator/disableIndicator.token");
    DISABLE_AUTO_UPGRADE_INTEGRATION_TOKEN.setFileName("/etc/lliurex-up-indicator/disableAutoUpgradeIntegration.token");

    connect(m_utils,&LliurexUpIndicatorUtils::startWidgetFinished,this,&LliurexUpIndicator::handleStartFinished);
    connect(m_timer, &QTimer::timeout, this, &LliurexUpIndicator::worker);
    connect(m_utils, &LliurexUpIndicatorUtils::updatesFound, this, &LliurexUpIndicator::handleUpdatesFoundFinished);
    connect(m_utils, &LliurexUpIndicatorUtils::unitStateChange, this, &LliurexUpIndicator::handleUnitStateChanged);

    m_watcher_timer->setSingleShot(true);
    m_watcher_timer->setInterval(500);
    connect(m_watcher_timer, &QTimer::timeout, this, &LliurexUpIndicator::updateStatus);

    QTimer::singleShot(0,this,[this](){
        m_utils->startWidget();
    });
    
}    

void LliurexUpIndicator::handleStartFinished(bool hideWidget){

    QString initTitle=i18n("No updates availables");
    setSubToolTip(initTitle);
    setCanLaunchLlxUp(false);

    if (!hideWidget){
        updatedInfo=true;
        if (!LliurexUpIndicator::DISABLE_AUTO_UPGRADE_INTEGRATION_TOKEN.exists()){
            m_utils->createSubscription();
            pauseWidget=false;
        }
    }

    m_timer->start(1200000);
    initWatcher();
    worker();
 
}

void LliurexUpIndicator::handleUnitStateChanged(bool autoUpgradeRun){

    bool forceUpdate=false;

    if (pauseWidget!=autoUpgradeRun){
        if (pauseWidget){
            forceUpdate=true;
        }
        pauseWidget=autoUpgradeRun;
    }
    
    if (pauseWidget && !isWorking){
        changeTryIconState(1,false);
    }else if (forceUpdate && !remoteUpdateInfo){
        updateCache();
    }
}

void LliurexUpIndicator::initWatcher(){

    if (!watcher) {
        watcher = new QFileSystemWatcher(this);
        connect(watcher, &QFileSystemWatcher::directoryChanged, [this](const QString &path){
            Q_UNUSED(path);
            m_watcher_timer->start();
        });
    }

    if (watcher->directories().isEmpty()) {
        watcher->addPath(m_utils->refPath);
    }

    updateStatus();
}

void LliurexUpIndicator::updateStatus() {

    if (LliurexUpIndicator::DISABLE_WIDGET_TOKEN.exists()) return;

    if (LliurexUpIndicator::TARGET_FILE.exists()) {
        if (!isWorking && !isCacheWorking){
            isWorking =true;
            remoteUpdateInfo = true;
            int iconState = m_utils->checkRemote() ? 2 : 1;
            changeTryIconState(iconState, iconState == 2);
        }
        return;
    }

    if (isWorking) {
        isWorking = false;
        remoteUpdateInfo = false;
        m_watcher_timer->stop();
        changeTryIconState(1,false);
    }
}

void LliurexUpIndicator::worker(){

    if (isWorking || LliurexUpIndicator::DISABLE_WIDGET_TOKEN.exists()){
        return;
    }

    if (updatedInfo && !remoteUpdateInfo) {
        lastUpdate += 1200;
        if (lastUpdate >= FREQUENCY || m_utils->isCacheUpdated()) {
            if (!pauseWidget){
                updateCache();
            }
        }
    }
}    

void LliurexUpIndicator::updateCache() {

    lastUpdate=0;
    
    if (!isCacheWorking && !isWorking){
        isCacheWorking = true;
        m_utils->runUpdateCache();
    }
}

void LliurexUpIndicator::handleUpdatesFoundFinished(bool result) {

    isCacheWorking = false;

    if (result) {
        changeTryIconState(0, true);
    }else{
        changeTryIconState(1,false);
    }
}  

void LliurexUpIndicator::changeTryIconState(int state, bool showNotification = true) {

    const QString tooltip = i18n("Lliurex-Up");
    bool isStudent = m_utils->isStudent;

    setCanLaunchLlxUp(false);

    switch(state) {
        case 0: {

            if (!isStudent){
                QString subToolTip = i18n("There are new packages ready to be updated or installed");
                QString icon = "lliurexupnotifier";
     
                setStatus(ActiveStatus);
                setIconName(icon);
                setSubToolTip(subToolTip);
                setToolTip(tooltip);
                setCanLaunchLlxUp(true);

                if (showNotification) {
                    m_updatesAvailableNotification = new KNotification(QStringLiteral("Update"), KNotification::CloseOnTimeout,this);
                    m_updatesAvailableNotification->setComponentName(QStringLiteral("llxupnotifier"));
                    m_updatesAvailableNotification->setTitle(subToolTip);
                    m_updatesAvailableNotification->setIconName(icon);
                    const QString name = i18n("Update now");
                    m_updatesAvailableNotification->setDefaultAction({name});
                    m_updatesAvailableNotification->setActions({name});
                    connect(m_updatesAvailableNotification, QOverload<unsigned int>::of(&KNotification::activated), this, &LliurexUpIndicator::launchLlxup);
                    m_updatesAvailableNotification->sendEvent();
                }
            }else {
                setStatus(PassiveStatus);
            }
            break;
        }
        case 1: { 
            setStatus(PassiveStatus);
            setIconName("lliurexupnotifier");
            QString lastExec = i18n("Last execution: ") + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
            setSubToolTip(lastExec);
            if (m_remoteUpdateNotification) m_remoteUpdateNotification->close();
            break;
        }
        case 2: { 
            setStatus(ActiveStatus);
            QString title = i18n("Lliurex-Up is being run remotely or automatically");
            QString body = i18n("Do not turn-off or restart your computer");
            QString icon = "lliurexupnotifier-running";

            setToolTip(tooltip);
            setSubToolTip(title + "\n" + body);
            setIconName(icon);

            m_remoteUpdateNotification = new KNotification(QStringLiteral("remoteUpdate"), KNotification::Persistent,this);
            m_remoteUpdateNotification->setComponentName(QStringLiteral("llxupnotifier"));
            m_remoteUpdateNotification->setTitle(title);
            m_remoteUpdateNotification->setText(body);
            m_remoteUpdateNotification->setIconName(icon);
            m_remoteUpdateNotification->sendEvent();
            break;
        }
    }
}

void LliurexUpIndicator::launchLlxup(){

    if (m_status==0){
        KIO::CommandLauncherJob *job = nullptr;
        QString cmd="lliurex-up-desktop-launcher.py";
        job = new KIO::CommandLauncherJob(cmd);
        job->start();
        if (m_updatesAvailableNotification){
            m_updatesAvailableNotification->close();
        }
    }    
}

LliurexUpIndicator::TrayStatus LliurexUpIndicator::status() const
{
    return m_status;
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

void LliurexUpIndicator::setCanLaunchLlxUp(bool canLaunchLlxUp)
{
    if (m_canLaunchLlxUp != canLaunchLlxUp){
        m_canLaunchLlxUp = canLaunchLlxUp;
        emit canLaunchLlxUpChanged();
    }
}
