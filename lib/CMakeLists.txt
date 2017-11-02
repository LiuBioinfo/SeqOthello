SET(LibTinyXML_SRC tinyxml2.cpp)
ADD_LIBRARY(tinyxml2 STATIC ${LibTinyXML_SRC})

SET(LibTCP_SRC socket.cpp)
ADD_LIBRARY(smalltcp STATIC ${LibTCP_SRC})
