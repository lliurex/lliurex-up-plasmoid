import QtQuick 2.15
import QtQuick.Layouts 1.15

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.1 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
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
            case LliurexUpIndicator.ActiveStatus:
                populateContextualActions()
                return PlasmaCore.Types.ActiveStatus
            case LliurexUpIndicator.PassiveStatus:
                populateContextualActions()
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
        populateContextualActions();
    }

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation
   
    Plasmoid.fullRepresentation: PlasmaComponents3.Page {
        implicitWidth: PlasmaCore.Units.gridUnit * 12
        implicitHeight: PlasmaCore.Units.gridUnit * 6

        PlasmaExtras.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (PlasmaCore.Units.gridUnit * 4)
            iconName: Plasmoid.icon
            text:Plasmoid.toolTipSubText
        }
    }


 
    function action_llxup() {
        lliurexUpIndicator.launchLlxup()
    }

    function action_cancelAutoUpdate() {
        lliurexUpIndicator.cancelAutoUpdate()
    }

    function populateContextualActions() {
        plasmoid.clearActions()

        plasmoid.setAction("llxup", i18n("Update the system"), "lliurex-up")
        plasmoid.action("llxup").enabled = lliurexUpIndicator.canLaunchLlxUp
        plasmoid.action("llxup").visible = lliurexUpIndicator.canLaunchLlxUp

        plasmoid.setAction("cancelAutoUpdate", i18n("Cancel automatic update"),"chronometer-pause.svg")
        plasmoid.action("cancelAutoUpdate").enabled = lliurexUpIndicator.canStopAutoUpdate
        plasmoid.action("cancelAutoUpdate").visible = lliurexUpIndicator.canStopAutoUpdate
        
    }

 }	
