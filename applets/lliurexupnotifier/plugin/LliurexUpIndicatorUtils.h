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

#ifndef PLASMA_LLIUREX_UP_INDICATOR_UTILS_H
#define PLASMA_LLIUREX_UP_INDICATOR_UTILS_H

#include <QObject>
#include <QFile>
#include <QStringList>
#include <QtDBus/QDBusInterface>

#include "LliurexUpIndicatorServiceSubscriber.h"


class QTimer;

class LliurexUpIndicatorUtils : public QObject
{
    Q_OBJECT

public:

    static LliurexUpIndicatorUtils& instance(){
        static LliurexUpIndicatorUtils _instance;
        return _instance;
    }

    LliurexUpIndicatorUtils(const LliurexUpIndicatorUtils&)=delete;
    LliurexUpIndicatorUtils& operator=(const LliurexUpIndicatorUtils&) =delete;

    QString refPath = "/var/run";
    QString user;
    bool isStudent = false;

    void startWidget();
    void createSubscription();
    void runUpdateCache();
    bool checkRemote();
    bool isCacheUpdated();
    void checkUpdates();
    void runSimulation(const QString &transPath);

public slots:

    void onCacheFinished();

signals:

    void startWidgetFinished(bool showWidget);
    void updatesFound(bool hiHaActualitzacions);
    void unitStateChange(bool autoUpgradeRun);

private:

    explicit LliurexUpIndicatorUtils();

    QFile PKGCACHE;
    qint64 APT_FRECUENCY = 1200000;
    
    bool m_isStarting=false;
    bool m_isStarted=false;
    bool m_updatingCache=false;
    bool m_isCheckingUpdates=false;
    bool cacheUpdated = true;
    bool m_isCreatingSubscription=false;
    bool m_hideWidget=false;

    LliurexUpIndicatorServiceSubscriber* m_service;


    QDBusInterface *m_transIface = nullptr; 
    QString m_transPath;
    QStringList getFlavours();
    QStringList getUserGroups();
    bool isConnectionWithServer();

private slots:
     void handleCreateSubscriptionFinished(bool startOk);
     void handleManageService(bool autoUpgradeRun);
};

#endif // PLASMA_LLIUREX_UP_INDICATOR_UTILS_H
