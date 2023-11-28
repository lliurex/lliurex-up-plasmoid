project(plasma-widget-lliurex-up-notifier)
set(PROJECT_VERSION "5.12.8")
set(PROJECT_VERSION_MAJOR 5)

cmake_minimum_required(VERSION 2.8.12)

################# Disallow in-source build #################
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
   message(FATAL_ERROR "plasma requires an out of source build. Please create a separate build directory and run 'cmake path_to_plasma [options]' there.")
endif()

include(FeatureSummary)

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)

set(REQUIRED_QT_VERSION 5.9.0)
set(KF5_MIN_VERSION 5.59.0)
find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Core Gui DBus Quick Qml Widgets X11Extras)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS Plasma I18n Notifications KIO)
find_package(EdupalsBase REQUIRED)
find_package(EdupalsN4D REQUIRED)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fexceptions")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -foperator-names")


find_package(X11)
set_package_properties(X11 PROPERTIES DESCRIPTION "X11 libraries"
                        URL "http://www.x.org"
                        TYPE REQUIRED
                        PURPOSE "Required for building the X11 based workspace"
                      )

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

#########################################################################

################# list the subdirectories #################
add_subdirectory(applets)

configure_file(config-plasma-widget-lliurex-up-notifier.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-plasma-widget-lliurex-up-notifier.h)

configure_file(config-X11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-X11.h)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

find_package(KF5I18n CONFIG REQUIRED)
ki18n_install(po)
