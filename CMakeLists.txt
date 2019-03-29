cmake_minimum_required(VERSION 3.10)
project(8 VERSION 0.1.0)

add_compile_options(
  $<$<CONFIG:Debug>:-g>
  -Wall
  -Wextra
  -Werror=int-to-pointer-cast
  -Werror=incompatible-pointer-types
  -Werror=pointer-to-int-cast
  -Werror=int-conversion
  -Warray-bounds
  -Wformat-overflow
  # -Wvla
  # -Wmissing-field-initializers
  # -Wsizeof-array-argument
  # -Wtype-limits
  $<$<CONFIG:Debug>:-fsanitize=undefined,address>)
add_link_options(
  $<$<CONFIG:Debug>:-fsanitize=undefined,address>)

add_subdirectory(src)
add_subdirectory(test)
