cmake_minimum_required(VERSION 2.8.12)

project(KDialog)

# ECM setup
find_package(ECM 1.7.0 CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

# Build dependencies
find_package(KF5 REQUIRED COMPONENTS KDELibs4Support KIO)
find_package(Qt5DBus CONFIG)

find_package(X11)
if(X11_FOUND)
   set(HAVE_X11 1)
endif()

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
