
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )

set(SOURCES 
       av/demuxer.cpp
       av/av-stream.cpp
       av/audio.cpp
       av/audio-buffer.cpp
       av/audio-source.cpp
       av/audio-stream.cpp
       av/video-stream.cpp
       )
       
install(
    FILES
       demuxer.h
       av-stream.h
       audio.h
       audio-buffer.h
       audio-source.h
       audio-stream.h
       video-stream.h
    DESTINATION ${HEADER_INSTALL_PATH}/av
    )
