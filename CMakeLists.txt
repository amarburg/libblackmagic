cmake_minimum_required(VERSION 3.5)
project(libblackmagic)

if( FIPS_CONFIG AND NOT FIPS_IMPORT )
	cmake_minimum_required(VERSION 3.5)

	get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
	include("${FIPS_ROOT_DIR}/cmake/fips.cmake")

	fips_setup()
else()
  find_package(catkin QUIET)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

## Default OpenCV version
list(APPEND OPENCV_COMPONENTS core highgui imgproc )
find_package(OpenCV COMPONENTS ${OPENCV_COMPONENTS} REQUIRED)

## C++ flags applied to all builds
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

## Configure OpenCV ##

## Find Blackmagic SDK
set( BLACKMAGIC_FORCE_LINUX TRUE )
find_package( BlackmagicSDK REQUIRED )

## Fips build path
if (FIPS_CONFIG)
	## Set global include paths
	fips_include_directories(
	  ${CMAKE_SOURCE_DIR}/include
	  ${BLACKMAGIC_INCLUDE_DIR}
	  ${OpenCV_INCLUDE_DIRS}
	 )


	fips_add_subdirectory("lib/")

	if( NOT FIPS_IMPORT )
	  add_subdirectory("tools/")

		if( FIPS_UNITTESTS )
				gtest_begin( blackmagic )
				    fips_src( test/unit/ )
				    fips_deps( blackmagic )
				gtest_end()
		endif()

	  fips_finish()
	endif()

## Catkin build path
else()
  project(libblackmagic)

  find_package(catkin REQUIRED
    g3log_catkin
    active_object
    libbmsdi
  )

  catkin_package(
    #CATKIN_DEPENDS g3log_catkin
    INCLUDE_DIRS include ${BLACKMAGIC_INCLUDE_DIR}
    LIBRARIES blackmagic
  )

  include_directories(
      include
      ${BLACKMAGIC_INCLUDE_DIR}
      ${OpenCV_INCLUDE_DIRS}
      ${catkin_INCLUDE_DIRS}
  )

    file(GLOB libblackmagic_srcs lib/*.cpp )

    add_library(blackmagic ${libblackmagic_srcs} ${BLACKMAGIC_SRCS} )
    target_link_libraries(blackmagic  ${catkin_LIBRARIES})

    install(TARGETS blackmagic
          ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
          LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
          RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION})

    ## Install headers
    install(DIRECTORY  include/${PROJECT_NAME}/  #${BLACKMAGIC_INCLUDE_DIR}
        DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
        PATTERN ".git" EXCLUDE)





endif()
