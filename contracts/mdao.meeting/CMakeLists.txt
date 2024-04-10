add_contract(mdao.meeting mdao.meeting ${CMAKE_CURRENT_SOURCE_DIR}/src/mdao.meeting.cpp)

target_include_directories(mdao.meeting
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include/mdao.meeting
   ${CMAKE_CURRENT_SOURCE_DIR}/../mdao.thirdparty

)

set_target_properties(mdao.meeting
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

target_compile_options( mdao.meeting PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
