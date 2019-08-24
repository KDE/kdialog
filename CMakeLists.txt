cmake_minimum_required(VERSION 3.0)

# KDE Application Version, managed by release script
set(KDE_APPLICATIONS_VERSION_MAJOR "19")
set(KDE_APPLICATIONS_VERSION_MINOR "11")
set(KDE_APPLICATIONS_VERSION_MICRO "70")
set(KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")

project(KDialog VERSION ${KDE_APPLICATIONS_VERSION})

# ECM setup
find_package(ECM 1.7.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)

# Build dependencies
find_package(KF5 5.62 REQUIRED COMPONENTS TextWidgets Notifications GuiAddons IconThemes WindowSystem KIO DBusAddons)
find_package(Qt5DBus CONFIG)

find_package(X11)
if(X11_FOUND)
   set(HAVE_X11 1)
endif()

add_definitions(
    -DQT_DEPRECATED_WARNINGS
    -DQT_USE_QSTRINGBUILDER
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_STRICT_ITERATORS
    -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_SIGNALS_SLOTS_KEYWORDS
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
)

add_subdirectory(src)
install(FILES org.kde.kdialog.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
