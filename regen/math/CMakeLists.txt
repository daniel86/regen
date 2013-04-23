
set(SOURCES
    math/vector.cpp
    math/matrix.cpp
    math/frustum.cpp
    )

install(
    FILES
        math.h
        frustum.h
        matrix.h
        quaternion.h
        vector.h
    DESTINATION ${HEADER_INSTALL_PATH}/algebra
)
