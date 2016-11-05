configure_file(config-kdialog.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kdialog.h)

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
