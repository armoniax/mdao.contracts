add_contract(mdao.groupthr mdao.groupthr ${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.groupthr.cpp)

target_include_directories(mdao.groupthr
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.conf/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.stg/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.info/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.gov/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.stake/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.thirdparty


)

set_target_properties(mdao.groupthr
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( mdao.groupthr PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
