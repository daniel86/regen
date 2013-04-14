
SET(QT_APP_HEADERS qt-gl-widget.h shader-input-widget.h)
QT4_WRAP_CPP(QT_APP_HEADERS_MOC ${QT_APP_HEADERS})

add_library( qt-application STATIC
       qt-gl-widget.cpp
       shader-input-widget.cpp
       qt-application.cpp
       ${QT_APP_HEADERS_MOC}
       )

