add_compile_options(-Wall -Wextra)

add_library(csapp csapp.c csapp.h)

add_executable(ftp_client ftp_client.c ftp_com.h)
target_link_libraries(ftp_client
                      PRIVATE csapp pthread)

add_executable(ftp_server ftp_server.c ftp_com.h)
target_link_libraries(ftp_server
                      PRIVATE csapp pthread)
