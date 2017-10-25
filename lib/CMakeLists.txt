SET(LibTinyXML_SRC tinyxml2.cpp)
ADD_LIBRARY(tinyxml2 STATIC ${LibTinyXML_SRC})

SET(LibMyTCP_SRC PracticalSocket.cpp)
ADD_LIBRARY(mytcp STATIC ${LibMyTCP_SRC})
