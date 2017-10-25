find_package( Boost REQUIRED COMPONENTS system thread)
include_directories( ${Boost_INCLUDE_DIRS} )
add_executable( server server.cpp ) # Example application based on main.cpp
add_executable( client client.cpp ) # Example application based on main.cpp

# Alternatively you could use ${Boost_LIBRARIES} here.
target_link_libraries( server ${Boost_LIBRARIES} pthread)
target_link_libraries( client ${Boost_LIBRARIES} pthread)


