find_package(OpenSSL REQUIRED)

add_compile_options(-Wall
                    -Wextra
                    -Werror=int-to-pointer-cast
                    -Werror=pointer-to-int-cast
                    -Werror=int-conversion
                    -Wvla
                    -Wmissing-field-initializers
                    -Wsizeof-array-argument
                    -Wint-to-pointer-cast
                    -Wpointer-to-int-cast
                    -Wcast-qual
                    -Wcast-align
                    -Wcast-function-type
                    -Wtype-limits
                    -Warray-bounds
                    -Wformat-overflow
                    -Wstringop-overflow
                    -Wnonnull
                    -fsanitize=undefined,address)
add_link_options(-fsanitize=undefined,address)

add_library(csapp csapp.c csapp.h)

add_library(ftp_com ftp_com.c ftp_com.h)

add_library(utils utils.c utils.h)

add_library(format format.c format.h)

add_library(UI UI.c UI.h)

add_executable(ftp_client ftp_client.c)
target_link_libraries(ftp_client
                      PRIVATE csapp
                              ftp_com
                              UI
                              utils
                              format
                              OpenSSL::SSL
                              pthread)

add_executable(ftp_server ftp_server.c ftp_com.h)
target_link_libraries(ftp_server
                      PRIVATE csapp
                              ftp_com
                              format
                              OpenSSL::SSL
                              pthread)
