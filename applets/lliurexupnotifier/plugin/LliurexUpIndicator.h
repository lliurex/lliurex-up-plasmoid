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
#ifndef PLASMA_LLIUREX_UP_INDICATOR_H
#define PLASMA_LLIUREX_UP_INDICATOR_H

#include <QObject>
#include <QProcess>
#include <QPointer>
#include <KNotification>
#include <QDir>
#include <QFile>
#include <QThread>
#include <QFileSystemWatcher>

#include "LliurexUpIndicatorUtils.h"

class QTimer;
class KNotification;
class AsyncDbus;


class LliurexUpIndicator : public QObject
{
    Q_OBJECT


    Q_PROPERTY(TrayStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString toolTip READ toolTip NOTIFY toolTipChanged)
    Q_PROPERTY(QString subToolTip READ subToolTip NOTIFY subToolTipChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)
    Q_PROPERTY(bool canLaunchLlxUp READ canLaunchLlxUp NOTIFY canLaunchLlxUpChanged)
    Q_PROPERTY(bool canStopAutoUpdate READ canStopAutoUpdate NOTIFY canStopAutoUpdateChanged)
    Q_ENUMS(TrayStatus)

public:
    /**
     * System tray icon states.
     */
    enum TrayStatus {
        ActiveStatus = 0,
        PassiveStatus,
        NeedsAttentionStatus
    };

    LliurexUpIndicator(QObject *parent = nullptr);

    TrayStatus status() const;
    void changeTryIconState (int state,bool showNotification);
    void setStatus(TrayStatus status);

    QString toolTip() const;
    void setToolTip(const QString &toolTip);

    QString subToolTip() const;
    void setSubToolTip(const QString &subToolTip);

    QString iconName() const;
    void setIconName(const QString &name);

    bool canLaunchLlxUp();
    void setCanLaunchLlxUp(bool);

    bool canStopAutoUpdate();
    void setCanStopAutoUpdate(bool);

    bool runUpdateCache();
    void isAlive();
    void hideAutoUpdate();


public slots:
    
    void initWatcher();
    void worker();
    void isLliurexUpRunning();
    void checkLlxUp();
    void launchLlxup();
    void cancelAutoUpdate();

signals:
   
    void statusChanged();
    void toolTipChanged();
    void subToolTipChanged();
    void iconNameChanged();
    void canLaunchLlxUpChanged();
    void canStopAutoUpdateChanged();

private:

    AsyncDbus* adbus;
    void plasmoidMode();
    QTimer *m_timer = nullptr;
    QTimer *m_timer_run=nullptr;
    QTimer *m_timer_cache=nullptr;
    TrayStatus m_status = PassiveStatus;
    QString m_iconName = QStringLiteral("lliurexupnotifier");
    QString m_toolTip;
    QString m_subToolTip;
    bool m_canLaunchLlxUp=true;
    bool m_canStopAutoUpdate=false;
    QFile TARGET_FILE;
    QFile DISABLE_WIDGET_TOKEN;
    int FREQUENCY=3600;
    bool updatedInfo=false;
    bool remoteUpdateInfo=false;
    bool isWorking=false;
    int lastUpdate=0;
    bool rememberUpdate=true;
    bool thereAreUpdates=false;
    bool autoUpdatesDisplayed=false;
    LliurexUpIndicatorUtils* m_utils;
    QPointer<KNotification> m_updatesAvailableNotification;
    QPointer<KNotification> m_remoteUpdateNotification;
    QFileSystemWatcher *watcher = nullptr;
    QString refPath="/var/run";

private slots:

     void updateCache();
     void dbusDone(bool status);
     
};

/**
 * Class monitoring the file system quota.
 * The monitoring is performed through a timer, running the 'quota'
 * command line tool.
 */

class AsyncDbus: public QThread

{

    Q_OBJECT

public:
    
    LliurexUpIndicator* llxindicator;
    
    AsyncDbus(LliurexUpIndicator* lliurexupindicator)
     {
        llxindicator = lliurexupindicator;
     }

     void run() override
     {      

        bool result=llxindicator->runUpdateCache();
        emit message(result);

     }
     
signals:

    void message(bool);



};


#endif // PLASMA_LLIUREX_DISK_QUOTA_H
