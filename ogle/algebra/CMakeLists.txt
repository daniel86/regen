
set(SOURCES
    algebra/vector.cpp
    algebra/matrix.cpp
    algebra/frustum.cpp
    )
       
install(
    FILES
        frustum.h
        matrix.h
        quaternion.h
        vector.h
    DESTINATION ${HEADER_INSTALL_PATH}/algebra
    )
