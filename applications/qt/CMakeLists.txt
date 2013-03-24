
SET(QT_APP_HEADERS qt-gl-widget.h generic-data-window.h)
QT4_WRAP_CPP(QT_APP_HEADERS_MOC ${QT_APP_HEADERS})

add_library( qt-application STATIC
       qt-gl-widget.cpp
       generic-data-window.cpp
       qt-application.cpp
       ${QT_APP_HEADERS_MOC}
       )
