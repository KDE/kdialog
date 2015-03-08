cmake_minimum_required(VERSION 2.8.12)

project(KDialog)

# ECM setup
find_package(ECM 1.7.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings)

# Build dependencies
find_package(KF5 REQUIRED COMPONENTS KDELibs4Support KIO)
find_package(Qt5DBus CONFIG)

find_package(X11)
if(X11_FOUND)
   set(HAVE_X11 1)
endif()

configure_file(config-kdialog.h.cmake ${KDialog_BINARY_DIR}/config-kdialog.h)

remove_definitions(-DQT_NO_URL_CAST_FROM_STRING)

set(kdialog_SRCS
    kdialog.cpp
    widgets.cpp
    klistboxdialog.cpp
    progressdialog.cpp)

qt5_add_dbus_adaptor( kdialog_SRCS org.kde.kdialog.ProgressDialog.xml progressdialog.h KProgressDialog )

add_executable(kdialog ${kdialog_SRCS})

# Need libkfile due to the code that adjusts the geometry of the KDirSelectDialog
target_link_libraries(kdialog
    KF5::KIOWidgets
    KF5::KDELibs4Support
)

if(Qt5DBus_FOUND)
    target_link_libraries(kdialog Qt5::DBus)
endif()

if (HAVE_X11)
  target_link_libraries(kdialog ${X11_X11_LIB})
endif()

install(TARGETS kdialog ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.kdialog.ProgressDialog.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

if("${CMAKE_SOURCE_DIR}" STREQUAL "${KDialog_SOURCE_DIR}")
  feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
