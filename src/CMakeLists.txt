INCLUDE_DIRECTORIES(../lib ../seqothlib)
set(HEADERS ../lib/tinyxml2.h)

set(PREPROCESS_SRC preprocess.cc)
set(GROUP_SRC group.cc)
set(BUILD_SRC build.cc)

ADD_EXECUTABLE(PreProcess ${PREPROCESS_SRC})
TARGET_LINK_LIBRARIES(PreProcess tinyxml2)

ADD_EXECUTABLE(Group ${GROUP_SRC})
TARGET_LINK_LIBRARIES(Group tinyxml2)

#ADD_EXECUTABLE(Build ${BUILD_SRC})
#TARGET_LINK_LIBRARIES(Build tinyxml2 z)

