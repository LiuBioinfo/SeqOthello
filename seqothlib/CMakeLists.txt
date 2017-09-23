set(libL2Node_SRCS
    L2Node.hpp
    L2Node.cpp
)

# Declare the library
add_library(libL2Node STATIC
    ${libL2Node_SRCS}
)

# Specify here the include directories exported
# by this library
target_include_directories(libL2Node PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
