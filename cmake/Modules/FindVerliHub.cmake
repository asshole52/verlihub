# - Find VerliHub
# Find VerliHub and VerliApi libraries and includes.
# Once done this will define
#
#  VERLIHUB_INCLUDE_DIRS   - where to find cserverdc.h, etc.
#  VERLIHUB_LIBRARIES      - List of libraries when using VerliHub.
#  VERLIHUB_FOUND          - True if VerliHub found.
#
#  VERLIHUB_VERSION_STRING - The version of VerliHub found (x.y.z)
#  VERLIHUB_VERSION_MAJOR  - The major version of VerliHub
#  VERLIHUB_VERSION_MINOR  - The minor version of VerliHub
#  VERLIHUB_VERSION_PATCH  - The patch version of VerliHub
#  VERLIHUB_VERSION_TWEAK  - The tweak version of VerliHub


SET(VERLIHUB_FOUND 0)

find_program(VERLIHUB_CONFIG verlihub_config
	/usr/local/verlihub/bin/
	/usr/local/bin/
	/usr/bin/
)

if(VERLIHUB_CONFIG)
	IF(NOT VerliHub_FIND_QUIETLY)
		MESSAGE(STATUS "Using verlihub_config: ${VERLIHUB_CONFIG}")
	ENDIF(NOT VerliHub_FIND_QUIETLY)
	# Get include directories
	exec_program(${VERLIHUB_CONFIG} ARGS --include OUTPUT_VARIABLE MY_TMP)
	string(REGEX REPLACE "-I([^ ]*) ?" "\\1;" MY_TMP "${MY_TMP}")
	SET(VERLIHUB_ADD_INCLUDE_PATH ${MY_TMP} CACHE "VerliHub include paths" PATH)

	# Get libraries
	exec_program(${VERLIHUB_CONFIG} ARGS --libs OUTPUT_VARIABLE MY_TMP )
	SET(VERLIHUB_ADD_LIBRARIES "")
	string(REGEX MATCHALL "-l[^ ]*" VERLIHUB_LIB_LIST "${MY_TMP}")
	string(REGEX REPLACE "-l([^;]*)" "\\1" VERLIHUB_ADD_LIBRARIES "${VERLIHUB_LIB_LIST}")
	#string(REGEX REPLACE ";" " " VERLIHUB_ADD_LIBRARIES "${VERLIHUB_ADD_LIBRARIES}")

	set(VERLIHUB_ADD_LIBRARIES_PATH "")
	string(REGEX MATCHALL "-L[^ ]*" VERLIHUB_LIBDIR_LIST "${MY_TMP}")
	foreach(LIB ${VERLIHUB_LIBDIR_LIST})
		string(REGEX REPLACE "[ ]*-L([^ ]*)" "\\1" LIB "${LIB}")
		list(APPEND VERLIHUB_ADD_LIBRARIES_PATH "${LIB}")
	endforeach(LIB ${VERLIHUB_LIBDIR_LIST})
	
	# Get verlihub version
	exec_program(${VERLIHUB_CONFIG} ARGS --version OUTPUT_VARIABLE MY_TMP)
	string(REGEX MATCH "(([0-9]+)\\.([0-9]+)\\.([0-9]+)-?(.*))" MY_TMP "${MY_TMP}")

	SET(VERLIHUB_VERSION_MAJOR ${CMAKE_MATCH_2})
	SET(VERLIHUB_VERSION_MINOR ${CMAKE_MATCH_3})
	SET(VERLIHUB_VERSION_PATCH ${CMAKE_MATCH_4})
	SET(VERLIHUB_VERSION_TWEAK ${CMAKE_MATCH_5})
	SET(VERLIHUB_VERSION_STRING ${MY_TMP})
	MESSAGE(STATUS "VerliHub version: ${VERLIHUB_VERSION_STRING}")

else(VERLIHUB_CONFIG)
	IF(VerliHub_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "verlihub_config not found. Please install VerliHub or specify the path to VerliHub with -DWITH_VERLIHUB=/path/to/verlihub")
	ENDIF(VerliHub_FIND_REQUIRED)
endif(VERLIHUB_CONFIG)

#LIST(REMOVE_DUPLICATES ${VERLIHUB_ADD_INCLUDE_PATH})
SET(VERLIHUB_INCLUDE_DIRS "${VERLIHUB_ADD_INCLUDE_PATH}")

find_path(VERLIHUB_INCLUDE_DIR
	NAMES
	cserverdc.h
	PATHS
	${VERLIHUB_ADD_INCLUDE_PATH}
	/usr/include
	/usr/include/verlihub
	/usr/local/include
	/usr/local/include/verlihub
	/usr/local/verlihub/include
	DOC
	"Specify the directory containing cserverdc.h."
)

foreach(LIB ${VERLIHUB_ADD_LIBRARIES})
	find_library(FOUND${LIB}
		${LIB}
		PATHS
		${VERLIHUB_ADD_LIBRARIES_PATH}
		/usr/lib
		/usr/lib/verlihub
		/usr/local/lib
		/usr/local/lib/verlihub
		/usr/local/verlihub/lib
		DOC "Specify the location of the verlihub libraries."
	)

	IF(FOUND${LIB})
		LIST(APPEND VERLIHUB_LIBRARIES ${FOUND${LIB}})
	ELSE(FOUND${LIB})
		IF(VerliHub_FIND_REQUIRED)
			MESSAGE(FATAL_ERROR "${LIB} library not found. Please install VerliHub")
		ENDIF(VerliHub_FIND_REQUIRED)
	ENDIF(FOUND${LIB})
endforeach(LIB ${VERLIHUB_ADD_LIBRARIES})

IF(VERLIHUB_INCLUDE_DIR AND VERLIHUB_LIBRARIES)
	SET(VERLIHUB_FOUND TRUE)
ELSE(VERLIHUB_INCLUDE_DIR AND VERLIHUB_LIBRARIES)
	SET(VERLIHUB_FOUND FALSE)
ENDIF(VERLIHUB_INCLUDE_DIR AND VERLIHUB_LIBRARIES)

IF(VERLIHUB_FOUND)
	IF(NOT VerliHub_FIND_QUIETLY)
		MESSAGE(STATUS "Found VerliHub: ${VERLIHUB_INCLUDE_DIR}, ${VERLIHUB_LIBRARIES}")
	ENDIF(NOT VerliHub_FIND_QUIETLY)
ELSE(VERLIHUB_FOUND)
	IF(VerliHub_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "VerliHub not found. Please install VerliHub")
	ENDIF(VerliHub_FIND_REQUIRED)
ENDIF(VERLIHUB_FOUND)

mark_as_advanced(VERLIHUB_INCLUDE_DIR VERLIHUB_LIBRARIES)