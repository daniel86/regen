
SET(_HEADERS scene-display-widget.h)
QT5_WRAP_CPP(_HEADERS_MOC ${_HEADERS})

add_executable(
    ${PROJECT_NAME}-scene-display
    scene-display.cpp
    scene-display-widget.cpp
    ${_HEADERS_MOC}
)
target_compile_options(
        ${PROJECT_NAME}-scene-display
        PRIVATE -fPIC)
target_link_libraries(
    ${PROJECT_NAME}-scene-display
    qt-application
    ${PROJECT_NAME}
    ${QT_LIBRARIES}
)
