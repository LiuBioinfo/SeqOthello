INCLUDE_DIRECTORIES(../lib ../seqothlib)
set(HEADERS ../lib/tinyxml2.h)

set(PREPROCESS_SRC preprocess.cc)
set(GROUP_SRC group.cc)
set(BUILD_SRC build.cc)
set(QUERY_SRC query.cc)

ADD_EXECUTABLE(PreProcess ${PREPROCESS_SRC})
TARGET_LINK_LIBRARIES(PreProcess tinyxml2)

ADD_EXECUTABLE(Group ${GROUP_SRC})
TARGET_LINK_LIBRARIES(Group tinyxml2 libUtil)

ADD_EXECUTABLE(Build ${BUILD_SRC})
TARGET_LINK_LIBRARIES(Build tinyxml2 z pthread libL2Node libL1Node)

ADD_EXECUTABLE(Query ${QUERY_SRC})
TARGET_LINK_LIBRARIES(Query tinyxml2 z pthread libL2Node libL1Node smalltcp)

ADD_EXECUTABLE(Client client.cc)
TARGET_LINK_LIBRARIES(Client pthread smalltcp)

ADD_EXECUTABLE(Test testL1Node.cpp)
TARGET_LINK_LIBRARIES(Test tinyxml2 z pthread libL2Node libL1Node)

