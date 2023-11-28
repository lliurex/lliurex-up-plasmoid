#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
packageRoot=".."
plasmoidName=$(kreadconfig5 --file ${DIR}/../applets/lliurexupnotifier/package/metadata.desktop --group="Desktop Entry" --key="X-KDE-PluginInfo-Name")
website=$(kreadconfig5 --file ${DIR}/../applets/lliurexupnotifier/package/metadata.desktop --group="Desktop Entry" --key="X-KDE-PluginInfo-Website")
widgetName="${plasmoidName##*.}"
bugAddress="${website}"
projectName="plasma_applet_${plasmoidName}"
outFile="${projectName}.pot.new"

find ${packageRoot} -name '*.cpp' -o -name '*.h' -o -name '*.qml' |sort > "${DIR}/infiles.list"
xgettext --from-code=UTF-8 -C --kde -ci18n -ki18n:1 -ki18nc:1c,2 -ki18np:1,2 -ki18ncp:1c,2,3 \
    -ktr2i18n:1 -kki18n:1 -kki18nc:1c,2 -kki18np:1,2 -kki18ncp:1c,2,3 \
    --msgid-bugs-address="${bugAddress}" --files-from=infiles.list --width=200 \
    --package-name="${widgetName}" --package-version="" \
    -D "${packageRoot}" -D "${DIR}" -o "${outFile}" || { echo "[merge] error xgettext, aborting."; exit 1; }

rm ${DIR}/infiles.list