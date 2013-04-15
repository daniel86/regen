
set(SOURCES 
       av/audio.cpp
       av/audio-buffer.cpp
       av/audio-source.cpp
       av/audio-stream.cpp
       av/av-stream.cpp
       av/demuxer.cpp
       av/video-stream.cpp
       av/video-texture.cpp
       )

install(
    FILES
        audio.h
        audio-buffer.h
        audio-source.h
        audio-stream.h
        av-stream.h
        demuxer.h
        video-stream.h
        video-texture.h
    DESTINATION ${HEADER_INSTALL_PATH}/av
)
