
set(_HEADERS noise-widget.h)
QT5_WRAP_CPP(_HEADERS_MOC ${_HEADERS})

QT5_WRAP_UI(UI_HEADERS noise-gui.ui)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(noise-gui
        noise-gui.cpp
        noise-widget.cpp
    ${_HEADERS_MOC}
    ${UI_HEADERS}
)
target_compile_options(noise-gui PRIVATE -fPIC)
target_link_libraries(noise-gui
    qt-application
    ${PROJECT_NAME}
    Qt5::Core Qt5::Gui Qt5::OpenGL
)
