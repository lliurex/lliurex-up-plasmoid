
#######################################################################################
# Package
plasma_install_package(package org.kde.plasma.lliurexupnotifier)


#######################################################################################
# Notes Library
set(PLUGIN_TARGET "lliurexupnotifierplugin")
add_definitions(-DTRANSLATION_DOMAIN="plasma_applet_org.kde.plasma.lliurexupnotifier")
ecm_add_qml_module(${PLUGIN_TARGET} URI org.kde.plasma.private.lliurexupnotifier VERSION 2.0)

target_sources(${PLUGIN_TARGET} PRIVATE
    plugin/plugin.cpp
    plugin/LliurexUpIndicator.cpp
    plugin/LliurexUpIndicatorUtils.cpp
    plugin/plugin.h
    plugin/LliurexUpIndicator.h
    plugin/LliurexUpIndicatorUtils.h
   
   
)

target_link_libraries(${PLUGIN_TARGET} PRIVATE
                      Qt6::Quick
                      Qt6::Core
                      Qt6::Concurrent
                      Qt6::Widgets
                      KF6::CoreAddons
                      KF6::I18n
                      KF6::Notifications
                      KF6::KIOWidgets
                      Edupals::Base Edupals::N4D
                      )

target_include_directories(${PLUGIN_TARGET} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/plugin)  
ecm_finalize_qml_module(lliurexupnotifierplugin)

install(TARGETS ${PLUGIN_TARGET} DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/private/lliurexupnotifier)

install(FILES "icons/lliurexupnotifier.svg"
        DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps/)
install(FILES "icons/lliurexupnotifier-running.svg"
        DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps/)
install(FILES "icons/lliurexupnotifier-autoupdate.svg"
        DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps/)
install(FILES "knotifications6/llxupnotifier.notifyrc"
        DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})

