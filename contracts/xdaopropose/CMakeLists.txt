add_contract(xdao.propose xdao.propose ${CMAKE_CURRENT_SOURCE_DIR}/src/xdao.propose.cpp)

target_include_directories(xdao.propose
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdaoconf/include   
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdaostg/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdaoinfo/include


)

set_target_properties(xdao.propose
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( xdao.propose PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
