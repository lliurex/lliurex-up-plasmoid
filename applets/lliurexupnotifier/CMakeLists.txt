
#######################################################################################
# Package
plasma_install_package(package org.kde.plasma.lliurexupnotifier)


#######################################################################################
# Notes Library
add_definitions(-DTRANSLATION_DOMAIN="plasma_applet_org.kde.plasma.lliurexupnotifier")
ecm_add_qml_module(lliurexupnotifierplugin URI org.kde.plasma.private.lliurexupnotifier)

target_sources(lliurexupnotifierplugin PRIVATE
    plugin/plugin.cpp
    plugin/LliurexUpIndicator.cpp
    plugin/LliurexUpIndicatorUtils.cpp
   
)

target_link_libraries(lliurexupnotifierplugin PRIVATE
                      Qt6::Quick
                      Qt6::Core
                      Qt6::Widgets
                      KF6::CoreAddons
                      KF6::I18n
                      KF6::Notifications
                      KF6::KIOWidgets
                      Edupals::Base Edupals::N4D
                      )

ecm_finalize_qml_module(lliurexupnotifierplugin)

install(FILES "icons/lliurexupnotifier.svg"
        DESTINATION ${DATAROOTDIR}/usr/share/icons/hicolor/scalable/apps/)
install(FILES "icons/lliurexupnotifier-running.svg"
        DESTINATION ${DATAROOTDIR}/usr/share/icons/hicolor/scalable/apps/)
install(FILES "icons/lliurexupnotifier-autoupdate.svg"
        DESTINATION ${DATAROOTDIR}/usr/share/icons/hicolor/scalable/apps/)
install(FILES "knotifications6/llxupnotifier.notifyrc"
        DESTINATION ${DATAROOTDIR}/usr/share/knotifications6/)

