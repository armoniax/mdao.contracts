add_contract(xdao.gov xdao.gov ${CMAKE_CURRENT_SOURCE_DIR}/src/xdao.gov.cpp)

target_include_directories(xdao.gov
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdao.conf/include   
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdaostg/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../xdao.info/include

)

set_target_properties(xdao.gov
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( xdao.gov PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
