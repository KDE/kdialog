ecm_setup_version(${RELEASE_SERVICE_VERSION}
    VARIABLE_PREFIX KDIALOG
    VERSION_HEADER kdialog_version.h
)

configure_file(config-kdialog.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-kdialog.h)

add_executable(kdialog)

target_sources(kdialog PRIVATE
    kdialog.cpp
    klistboxdialog.cpp
    klistboxdialog.h
    widgets.cpp
    widgets.h
    utils.cpp
    utils.h
)

target_link_libraries(kdialog
    KF6::KIOWidgets
    KF6::TextWidgets
    KF6::Notifications
    KF6::IconThemes
    KF6::GuiAddons
    KF6::WindowSystem
    KF6::IconWidgets
)

target_link_libraries(kdialog Qt::DBus)

if(HAVE_X11)
  target_link_libraries(kdialog ${X11_X11_LIB})
endif()

install(TARGETS kdialog ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.kde.kdialog.ProgressDialog.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR})

################################################################################

add_executable(kdialog_progress_helper)

qt_add_dbus_adaptor(progressdialog_dbus_adaptor_SRCS
    org.kde.kdialog.ProgressDialog.xml progressdialog.h ProgressDialog
)

target_sources(kdialog_progress_helper PRIVATE
    ${progressdialog_dbus_adaptor_SRCS}
    kdialog_progress_helper.cpp
    utils.cpp
    utils.h
    progressdialog.cpp
    progressdialog.h
)

target_link_libraries(kdialog_progress_helper
    KF6::DBusAddons
    KF6::I18n
    Qt::Widgets
)

if(HAVE_X11)
  target_link_libraries(kdialog_progress_helper ${X11_X11_LIB})
endif()

install(TARGETS kdialog_progress_helper ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES org.kde.kdialog.desktop DESTINATION ${KDE_INSTALL_APPDIR})
