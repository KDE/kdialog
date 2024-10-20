cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "24")
set(RELEASE_SERVICE_VERSION_MINOR "11")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(KDialog VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "6.7.0")
set(KF_MIN_VERSION "6.3.0")

# ECM setup
find_package(ECM ${KF_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMDeprecationSettings)
include(ECMSetupVersion)
include(FeatureSummary)
# Build dependencies
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS TextWidgets Notifications GuiAddons IconThemes WindowSystem KIO DBusAddons)
find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS DBus)

option(WITHOUT_X11 "Build without X11 integration (skips finding X11)" OFF)
if(NOT WITHOUT_X11)
    find_package(X11)
    set(HAVE_X11 ${X11_FOUND})
endif()

ecm_set_disabled_deprecation_versions(
    QT 6.8.0
    KF 6.7
)

add_subdirectory(src)
install(FILES org.kde.kdialog.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
ki18n_install(po)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
