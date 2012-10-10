
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fPIC"  )
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} -fPIC"  )

set(SOURCES)
set(BUFFER)

foreach(DIR
            rapidxml
            glsw)
    add_subdirectory(${DIR})
    get_directory_property(BUFFER DIRECTORY ${DIR} DEFINITION SOURCES)
    list(APPEND SOURCES ${BUFFER})
endforeach(DIR)

# add_library(ogle-external SHARED ${SOURCES})
