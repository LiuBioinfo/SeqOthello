file(GLOB SRCS *.cpp)

INCLUDE_DIRECTORIES(../../lib ../../seqothlib)

ADD_EXECUTABLE(testL2Node testL2Node.cpp main.cpp)
ADD_EXECUTABLE(testL1Node testL1Node.cpp main.cpp)

TARGET_LINK_LIBRARIES(testL2Node
    libL2Node
    libgtest
    libgmock
    z
)

TARGET_LINK_LIBRARIES(testL1Node
    libL1Node
    libgtest
    libgmock
    z
)
add_test(NAME testfoo
         COMMAND testfoo)
