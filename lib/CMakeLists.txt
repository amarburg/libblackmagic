# Set source files

fips_begin_module( blackmagic )
  fips_src( . )

  ## The Blackmagic API is distributed as source and will be compiled into
  ## this library
  fips_files(
    ${BLACKMAGIC_SRCS}
  )

  fips_deps( bmsdi_protocol logger )
  fips_libs( ${OpenCV_LIBS} dl )

  #  fips_include_directories(
  #  ${CMAKE_CURRENT_SOURCE_DIR}
  #  ${CMAKE_SOURCE_DIR}/include/libblackmagic
  #)
fips_end_module()
