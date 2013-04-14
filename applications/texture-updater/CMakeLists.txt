
SET(_HEADERS texture-updater-widget.h)
QT4_WRAP_CPP(_HEADERS_MOC ${_HEADERS})

add_executable(
    ${PROJECT_NAME}-texture-updater
    texture-updater.cpp
    texture-updater-widget.cpp
    ${_HEADERS_MOC}
)
target_link_libraries(
    ${PROJECT_NAME}-texture-updater
    qt-application
    ${PROJECT_NAME}
    ${QT_LIBRARIES}
    ${REGEN_LIBRARIES}
)

