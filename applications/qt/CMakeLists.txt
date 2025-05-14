
set(QT_APP_HEADERS scene-widget.h shader-input-widget.h)
QT5_WRAP_CPP(QT_APP_HEADERS_MOC ${QT_APP_HEADERS})

QT5_WRAP_UI(UI_HEADERS
    VectorWidget.ui
    ColorWidget.ui
    shader-input-editor.ui)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library( qt-application STATIC
        scene-widget.cpp
       RegenWidget.cpp
       RegenWidgetGL.cpp
       VectorWidget.cpp
       ColorWidget.cpp
       TextureWidget.cpp
       shader-input-widget.cpp
       qt-application.cpp
       qt-camera-events.cpp
       ${QT_APP_HEADERS_MOC}
       ${UI_HEADERS}
       )

