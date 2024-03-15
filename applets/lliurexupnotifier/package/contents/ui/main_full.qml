import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.components 2.0 as Components
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.12 as Kirigami


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
                if (Plasmoid.icon=="lliurexupnotifier-running"){
                    plasmoid.removeAction("llx-up")
                }else{
                    plasmoid.setAction("llxup", i18n("Update the system"), "lliurex-up")
                }
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
    }

    Plasmoid.fullRepresentation:Item{

        implicitWidth: Kirigami.Units.gridUnit*12
        implicitHeight: Kirigami.Units.gridUnit*12

        Kirigami.PlaceholderMessage{
            anchors.centerIn:parent
            width:parent.width
            icon.name:Plasmoid.icon
            text:Plasmoid.toolTipSubText
        }
    }

    function action_llxup() {
        if (Plasmoid.icon!="lliurexupnotifier-running"){
            lliurexUpIndicator.launch_llxup()
        }
    }

 }	
