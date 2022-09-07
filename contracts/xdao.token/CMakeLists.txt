add_contract(xdaotoken111 xdao.token ${CMAKE_CURRENT_SOURCE_DIR}/src/xdao.token.cpp)

target_include_directories(xdao.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdao.conf/include   
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdao.stg/include
   )
set_target_properties(xdao.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( xdao.token PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
