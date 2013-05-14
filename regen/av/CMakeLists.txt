
set(SOURCES 
       av/audio.cpp
       av/av-stream.cpp
       av/demuxer.cpp
       av/video-stream.cpp
       av/video-texture.cpp
       )

install(
    FILES
        audio.h
        av-stream.h
        demuxer.h
        video-stream.h
        video-texture.h
    DESTINATION ${HEADER_INSTALL_PATH}/av
)
