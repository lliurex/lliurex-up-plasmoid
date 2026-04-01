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
    bool cacheUpdated = true;
    bool isStudent = false;

    void startWidget();
    bool runUpdateCache();
    bool simulateUpgrade();
    bool checkRemote();
    bool isCacheUpdated();
    bool isAutoUpdateReady();
    bool isAutoUpdateRun();
    QString getAutoUpdateTime();
    bool canStopAutoUpdate();
    void stopAutoUpdate();

signals:
    void startWidgetFinished(bool showWidget);

private:    
    QFile PKGCACHE;
    qint64 APT_FRECUENCY = 1200000;
    
    QStringList getFlavours();
    QStringList getUserGroups();
    bool isConnectionWithServer();
};

#endif // PLASMA_LLIUREX_UP_INDICATOR_UTILS_H
