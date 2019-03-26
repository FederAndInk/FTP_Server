cmake_minimum_required(VERSION 3.10)
project(8 VERSION 0.1.0)

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

add_subdirectory(src)
add_subdirectory(test)
