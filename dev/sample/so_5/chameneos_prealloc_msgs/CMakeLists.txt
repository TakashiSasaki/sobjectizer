set(SAMPLE sample.so_5.chameneos_prealloc_msgs)
add_executable(${SAMPLE} main.cpp)
target_link_libraries(${SAMPLE} sobjectizer::SharedLib -latomic)
install(TARGETS ${SAMPLE} DESTINATION bin)

set(SAMPLE_S sample.so_5.chameneos_prealloc_msgs_s)
add_executable(${SAMPLE_S} main.cpp)
target_link_libraries(${SAMPLE_S} sobjectizer::StaticLib -latomic)
install(TARGETS ${SAMPLE_S} DESTINATION bin)
