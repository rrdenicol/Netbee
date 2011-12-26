# Set minimum version required.
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.7)

PROJECT(makealltools)

##### Code profiling by Google
OPTION(
  ENABLE_GOOGLE_CPU_PROFILER
  "Link against the Google Cpu Profiler performance tool library (it must be installed and available to the linker)"
  OFF
)

IF(ENABLE_GOOGLE_CPU_PROFILER)
  SET (CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-lprofiler")
ENDIF(ENABLE_GOOGLE_CPU_PROFILER)
#########################################

ADD_SUBDIRECTORY(nbeedump)
ADD_SUBDIRECTORY(nbsvchelper)
ADD_SUBDIRECTORY(nbsvcinstaller)
ADD_SUBDIRECTORY(nbextractor)

