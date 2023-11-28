
#######################################################################################
# Package
plasma_install_package(package org.kde.plasma.lliurexupnotifier)


#######################################################################################
# Notes Library
add_definitions(-DTRANSLATION_DOMAIN="plasma_applet_org.kde.plasma.lliurexupnotifier")

set(upnotifier_SRCS
    plugin/plugin.cpp
    plugin/LliurexUpIndicator.cpp
    plugin/LliurexUpIndicatorUtils.cpp
   
)

add_library(lliurexupnotifierplugin SHARED ${upnotifier_SRCS})

target_link_libraries(lliurexupnotifierplugin
                      Qt5::Quick
                      Qt5::Core
                      Qt5::Widgets
                      KF5::CoreAddons
                      KF5::I18n
                      KF5::Notifications
                      KF5::KIOWidgets
                      Edupals::Base Edupals::N4D
                      )

install(FILES plugin/qmldir DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/lliurexupnotifier)
install(TARGETS lliurexupnotifierplugin DESTINATION ${QML_INSTALL_DIR}/org/kde/plasma/private/lliurexupnotifier)

install(FILES "icons/lliurexupnotifier.svg"
        #DESTINATION ${DATAROOTDIR}/usr/share/plasma/desktoptheme/default/icons/)
        DESTINATION ${DATAROOTDIR}/usr/share/icons/hicolor/scalable/apps/)
install(FILES "icons/lliurexupnotifier-running.svg"
        DESTINATION ${DATAROOTDIR}/usr/share/icons/hicolor/scalable/apps/)
install(FILES "knotifications5/llxupnotifier.notifyrc"
        DESTINATION ${DATAROOTDIR}/usr/share/knotifications5/)        
#install(FILES "autostart/lliurex-up-indicator"
#        DESTINATION ${DATAROOTDIR}/etc/bin)        
