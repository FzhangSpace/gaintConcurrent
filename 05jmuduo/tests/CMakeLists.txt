add_executable(bsa bsa.cc)

add_executable(pthread_atfork_test Pthread_atfork_test.cc)
target_link_libraries(pthread_atfork_test pthread)
