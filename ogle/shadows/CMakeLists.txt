
set(SOURCES
        shadows/directional-shadow-map.cpp
        shadows/spot-shadow-map.cpp
        shadows/point-shadow-map.cpp
)

install(
    FILES
        directional-shadow-map.h
        spot-shadow-map.h
        point-shadow-map.h
    DESTINATION ${HEADER_INSTALL_PATH}/shadows
    )
