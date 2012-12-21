
set(SOURCES
        meshes/rectangle.cpp
        meshes/box.cpp
        meshes/sphere.cpp
        meshes/sky.cpp
        meshes/text.cpp
        meshes/particles.cpp
)

install(
    FILES
        rectangle.h
        box.h
        sky.h
        sphere.h
        text.h
        particles.h
    DESTINATION ${HEADER_INSTALL_PATH}/models
    )
