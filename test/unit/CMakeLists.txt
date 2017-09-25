file(GLOB SRCS *.cpp)

INCLUDE_DIRECTORIES(../../lib ../../seqothlib)

ADD_EXECUTABLE(testL2Node ${SRCS})

TARGET_LINK_LIBRARIES(testL2Node
    libL2Node
    libgtest
    libgmock
    z
)

add_test(NAME testfoo
         COMMAND testfoo)
