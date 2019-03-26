find_package(OpenSSL REQUIRED)

add_library(csapp csapp.c csapp.h)
target_link_libraries(csapp PRIVATE pthread)

add_library(ftp_com ftp_com.c ftp_com.h)
target_link_libraries(ftp_com PRIVATE m csapp OpenSSL::SSL)
target_include_directories(
  ftp_com
  PUBLIC ${CMAKE_CURRENT_LIST_DIR})

add_library(utils utils.c utils.h)

add_library(format format.c format.h)

add_library(UI UI.c UI.h)

add_executable(ftp_client ftp_client.c)
target_link_libraries(ftp_client
                      PRIVATE csapp
                              ftp_com
                              UI
                              utils
                              format)

add_executable(ftp_server ftp_server.c ftp_com.h)
target_link_libraries(ftp_server
                      PRIVATE csapp
                              ftp_com
                              format)
