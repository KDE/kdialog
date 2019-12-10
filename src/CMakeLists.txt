ecm_setup_version(${RELEASE_SERVICE_VERSION} VARIABLE_PREFIX KDIALOG VERSION_HEADER kdialog_version.h)

configure_file(config-kdialog.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kdialog.h)

set(kdialog_SRCS
    kdialog.cpp
    widgets.cpp
    utils.cpp
    klistboxdialog.cpp
)

add_executable(kdialog ${kdialog_SRCS})

target_link_libraries(kdialog
    KF5::KIOWidgets
    KF5::TextWidgets
    KF5::Notifications
    KF5::IconThemes
    KF5::GuiAddons
    KF5::WindowSystem
)

if(Qt5DBus_FOUND)
    target_link_libraries(kdialog Qt5::DBus)
endif()

if (HAVE_X11)
  target_link_libraries(kdialog ${X11_X11_LIB})
endif()

install(TARGETS kdialog ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.kdialog.ProgressDialog.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

################################################################################

set(kdialog_progress_helper_SRCS
    kdialog_progress_helper.cpp
    utils.cpp
    progressdialog.cpp
)

qt5_add_dbus_adaptor(kdialog_progress_helper_SRCS org.kde.kdialog.ProgressDialog.xml progressdialog.h ProgressDialog)

add_executable(kdialog_progress_helper ${kdialog_progress_helper_SRCS})

target_link_libraries(kdialog_progress_helper
    KF5::DBusAddons
    KF5::I18n
    Qt5::Widgets
)

if (HAVE_X11)
  target_link_libraries(kdialog_progress_helper ${X11_X11_LIB})
endif()

install(TARGETS kdialog_progress_helper ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

