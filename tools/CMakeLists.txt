
fips_begin_app( bm_viewer cmdline )
  fips_files(
    bm_viewer.cpp
  )

  fips_deps( blackmagic activeobject  )

  fips_include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
  )

fips_end_app()
