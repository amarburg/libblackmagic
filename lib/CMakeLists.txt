# Set source files

fips_begin_module( blackmagic )
  fips_src( . )

  ## The Blackmagic API is distributed as source and will be compiled into
  ## this library
  fips_files(
    ${BLACKMAGIC_SRCS}
  )

  fips_deps( bmsdi-protocol g3logger )
  fips_libs( ${OpenCV_LIBS} dl )

fips_end_module()
