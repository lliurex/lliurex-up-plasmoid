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


class QTimer;

class LliurexUpIndicatorUtils : public QObject
{
    Q_OBJECT

public:
    explicit LliurexUpIndicatorUtils(QObject *parent = nullptr);

    QFile AUTO_UPDATE_TOKEN;
    QFile AUTO_UPDATE_RUN_TOKEN;
    QString refPath = "/var/run";
    QString user;
    bool isStudent = false;

    void startWidget();
    void runUpdateCache();
    bool checkRemote();
    bool isCacheUpdated();
    bool isAutoUpdateReady();
    bool isAutoUpdateRun();
    void checkUpdates();
    void runSimulation(const QString &transPath);
    QString getAutoUpdateTime();
    QString getAutoUpdatePause();
    bool canStopAutoUpdate();
    void stopAutoUpdate();

public slots:

    void onCacheFinished();

signals:

    void startWidgetFinished(bool showWidget);
    void updatesFound(bool hiHaActualitzacions);

private:    
    QFile PKGCACHE;
    qint64 APT_FRECUENCY = 1200000;
    
    bool m_updatingCache=false;
    bool cacheUpdated = true;
    QDBusInterface *m_transIface = nullptr; 
    QString m_transPath;
    QStringList getUserGroups();
    bool isConnectionWithServer();
};

#endif // PLASMA_LLIUREX_UP_INDICATOR_UTILS_H
