import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as Components
import org.kde.plasma.extras 2.0 as PlasmaExtras

import org.kde.plasma.private.lliurexupnotifier 1.0
// Item - the most basic plasmoid component, an empty container.
Item {

    id:lliurexupApplet

    LliurexUpIndicator{
        id:lliurexUpIndicator

    }

    Plasmoid.status: {
        /* Warn! Enum types are accesed through ClassName not ObjectName */
        switch (lliurexUpIndicator.status){
            case LliurexUpIndicator.NeedsAttentionStatus:
                plasmoid.removeAction("llx-up")
                return PlasmaCore.Types.NeedsAttentionStatus
            case LliurexUpIndicator.ActiveStatus:
                plasmoid.setAction("llxup", i18n("Update the system"), "update-low")
                return PlasmaCore.Types.ActiveStatus
            case LliurexUpIndicator.PassiveStatus:
                return PlasmaCore.Types.PassiveStatus
        }
        
        return  PlasmaCore.Types.ActiveStatus

    }

    Plasmoid.switchWidth: units.gridUnit * 5
    Plasmoid.switchHeight: units.gridUnit * 5

    Plasmoid.icon:lliurexUpIndicator.iconName
    Plasmoid.toolTipMainText: lliurexUpIndicator.toolTip
    Plasmoid.toolTipSubText: lliurexUpIndicator.subToolTip

    Component.onCompleted: {
        plasmoid.removeAction("configure");
        /*plasmoid.setAction("llxup", i18n("Update the system"), "update-low"); */
    }

    Plasmoid.preferredRepresentation: Plasmoid.compactRepresentation
    Plasmoid.compactRepresentation: PlasmaCore.IconItem {
        source: plasmoid.icon
        MouseArea {
            anchors.fill: parent
        }
    }

    Plasmoid.onExpandedChanged: if (Plasmoid.expanded) {
        action_llxup()
    }

    function action_llxup() {
        lliurexUpIndicator.launch_llxup()
    }

 }	
