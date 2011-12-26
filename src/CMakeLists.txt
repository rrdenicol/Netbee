# Set requirement for minimum version.
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#IF (CMAKE_MAJOR_VERSION GREATER 2)
#	# Do not try to escape preprocessor definition (default behavior before cmake 2.6).
#	CMAKE_POLICY(SET CMP0005 OLD)
#
#	# Split additional libraries specifications for the linker in path and filename 
#	# (e.g. "path/libname.so" becomes "-Lpath -lname" if gcc is used).
#	# This was the default behavior before cmake 2.6.
#	CMAKE_POLICY(SET CMP0003 OLD)
#	
#ELSE (CMAKE_MAJOR_VERSION GREATER 2)
#	IF (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION GREATER 4)
#		CMAKE_POLICY(SET CMP0005 OLD)
#		CMAKE_POLICY(SET CMP0003 OLD)
#	ENDIF (CMAKE_MAJOR_VERSION EQUAL 2 AND CMAKE_MINOR_VERSION GREATER 4)
#ENDIF (CMAKE_MAJOR_VERSION GREATER 2)

PROJECT(makeallsources)


IF(CMAKE_COMPILER_IS_GNUCXX)
        ADD_DEFINITIONS(-Wall -Wno-write-strings)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
IF(CMAKE_COMPILER_IS_GNUCC)
        ADD_DEFINITIONS(-Wall -Wno-write-strings)
ENDIF(CMAKE_COMPILER_IS_GNUCC)

# if you ever need to profile the library code, uncomment this
#ADD_DEFINITIONS(
#        -pg
#)
#SET (CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-pg")

ADD_SUBDIRECTORY(nbee)
ADD_SUBDIRECTORY(nbnetvm)
ADD_SUBDIRECTORY(nbpflcompiler)
ADD_SUBDIRECTORY(nbprotodb)
ADD_SUBDIRECTORY(nbsockutils)


# Set dependencies among project files
ADD_DEPENDENCIES(nbpflcompiler nbprotodb)
ADD_DEPENDENCIES(nbee nbprotodb nbpflcompiler nbnetvm nbsockutils)

