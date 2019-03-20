add_compile_options(-Wall -Wextra
                    -fsanitize=undefined,address)
add_link_options(-fsanitize=undefined,address)

add_library(csapp csapp.c csapp.h)

add_library(ftp_com ftp_com.c)

add_executable(ftp_client ftp_client.c)
target_link_libraries(ftp_client
                      PRIVATE csapp ftp_com
                              pthread)

add_executable(ftp_server ftp_server.c ftp_com.h)
target_link_libraries(ftp_server
                      PRIVATE csapp ftp_com
                              pthread)
