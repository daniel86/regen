
SET(QT_OGLE_HEADERS qt-gl-widget.h generic-data-window.h)
QT4_WRAP_CPP(QT_OGLE_HEADERS_MOC ${QT_OGLE_HEADERS})

add_library( qt-application STATIC
       ../ogle-application.cpp
       qt-gl-widget.cpp
       generic-data-window.cpp
       qt-application.cpp
       ${QT_OGLE_HEADERS_MOC}
       )
