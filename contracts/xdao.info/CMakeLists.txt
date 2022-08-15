add_contract(xdaoinfotest xdao.info ${CMAKE_CURRENT_SOURCE_DIR}/src/xdao.info.cpp)

target_include_directories(xdao.info
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include/xdao.info
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdaostg/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdao.conf/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdao.gov/include

)

set_target_properties(xdao.info
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( xdao.info PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
