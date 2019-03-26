
add_executable(test_seg_file test_seg_file.c)
target_link_libraries(test_seg_file
                      PRIVATE ftp_com)
