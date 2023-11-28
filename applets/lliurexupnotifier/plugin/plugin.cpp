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
#include "plugin.h"
#include "LliurexUpIndicator.h"
#include "LliurexUpIndicatorUtils.h"

#include <QtQml>

void LliurexUpIndicatorPlugin::registerTypes (const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.plasma.private.lliurexupnotifier"));
    qmlRegisterType<LliurexUpIndicator>(uri, 1, 0, "LliurexUpIndicator");
    qmlRegisterType<LliurexUpIndicatorUtils>(uri, 1, 0, "LliurexUpIndicatorUtils");
}
