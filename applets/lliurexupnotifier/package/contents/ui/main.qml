import QtQuick
import QtQuick.Layouts

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami

import org.kde.plasma.private.lliurexupnotifier 1.0
// Item - the most basic plasmoid component, an empty container.

PlasmoidItem {

    id:lliurexupApplet

    LliurexUpIndicator{
        id:lliurexUpIndicator
    }

    Plasmoid.status: {
        switch (lliurexUpIndicator.status){
            case LliurexUpIndicator.ActiveStatus:
                return PlasmaCore.Types.ActiveStatus
            case LliurexUpIndicator.PassiveStatus:
                return PlasmaCore.Types.PassiveStatus
            }
        return  PlasmaCore.Types.ActiveStatus
    }

    switchWidth: Kirigami.Units.gridUnit * 5
    switchHeight: Kirigami.Units.gridUnit * 5

    Plasmoid.icon:lliurexUpIndicator.iconName
    toolTipMainText: lliurexUpIndicator.toolTip
    toolTipSubText: lliurexUpIndicator.subToolTip

    Component.onCompleted: {
        Plasmoid.setInternalAction("configure", configureAction)

    }

  
    fullRepresentation: PlasmaComponents3.Page {
        implicitWidth: Kirigami.Units.gridUnit * 12
        implicitHeight: Kirigami.Units.gridUnit * 6

        PlasmaExtras.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.gridUnit * 4)
            iconName: Plasmoid.icon
            text:lliurexUpIndicator.subToolTip

        }
    }

    function action_llxup() {
        lliurexUpIndicator.launchLlxup()

    }

    function action_cancelAutoUpdate() {
        lliurexUpIndicator.cancelAutoUpdate()

    }

    Plasmoid.contextualActions: [
        PlasmaCore.Action{
            text: i18n("Wait until tomorrow")
            icon.name:"chronometer-pause.svg"
            visible:lliurexUpIndicator.canStopAutoUpdate
            enabled:lliurexUpIndicator.canStopAutoUpdate
            onTriggered:action_cancelAutoUpdate()
        }

    ]

    PlasmaCore.Action {
        id: configureAction
        text: i18n("Update the system")
        icon.name:"lliurex-up.svg"
        visible:lliurexUpIndicator.canLaunchLlxUp
        enabled:lliurexUpIndicator.canLaunchLlxUp
        onTriggered:action_llxup()
    }

 }	
