INCLUDE_DIRECTORIES(../lib ../seqothlib ../Jellyfish/include ../Jellyfish ../Jellyfish)
set(HEADERS ../lib/tinyxml2.h)

set(PREPROCESS_SRC preprocess.cc)
set(GROUP_SRC group.cc)
set(BUILD_SRC build.cc)
set(QUERY_SRC query.cc)
set(PRINT_SRC printrates.cc)

ADD_LIBRARY(Jellyfish_Json STATIC ../Jellyfish/lib/jsoncpp.cpp)
ADD_LIBRARY(Jellyfish_Matrix STATIC ../Jellyfish/config.h ../Jellyfish/lib/rectangular_binary_matrix.cc)
SET_TARGET_PROPERTIES(Jellyfish_Matrix PROPERTIES COMPILE_FLAGS "-DHAVE_CONFIG_H")
ADD_LIBRARY(Jellyfish_Mer_DNA STATIC ../Jellyfish/lib/mer_dna.cc)

ADD_EXECUTABLE(PreProcess ${PREPROCESS_SRC})
TARGET_LINK_LIBRARIES(PreProcess tinyxml2 Jellyfish_Json Jellyfish_Matrix Jellyfish_Mer_DNA)

ADD_EXECUTABLE(Group ${GROUP_SRC})
TARGET_LINK_LIBRARIES(Group pthread tinyxml2 libUtil)

ADD_EXECUTABLE(Build ${BUILD_SRC})
TARGET_LINK_LIBRARIES(Build tinyxml2 z pthread libL2Node libL1Node)

ADD_EXECUTABLE(Query ${QUERY_SRC})
TARGET_LINK_LIBRARIES(Query tinyxml2 z pthread libL2Node libL1Node smalltcp)

ADD_EXECUTABLE(PrintRates ${PRINT_SRC})
TARGET_LINK_LIBRARIES(PrintRates tinyxml2 z pthread libL2Node libL1Node smalltcp)

ADD_EXECUTABLE(Client client.cc)
TARGET_LINK_LIBRARIES(Client pthread smalltcp)

ADD_EXECUTABLE(Test testL1Node.cpp)
TARGET_LINK_LIBRARIES(Test tinyxml2 z pthread libL2Node libL1Node)


