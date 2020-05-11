
# Find the native Tcmalloc library
#
#  Tcmalloc_LIBRARIES   - List of libraries when using Tcmalloc.
#  Tcmalloc_FOUND       - True if Tcmalloc found.

set(TCMALLOC_NAMES
  tcmalloc
  tcmalloc_minimal
  )

find_library(Tcmalloc_LIBRARY
  NAMES ${TCMALLOC_NAMES}
  #PATHS
  #  /lib
  #  /lib64
  #  /usr/lib
  #  /usr/lib64
  #  /usr/local/lib
  #  /opt/local/lib64
  #  /usr/lib/x86_64-linux-gnu
)

if (Tcmalloc_LIBRARY)
  set(Tcmalloc_FOUND TRUE)
  set(Tcmalloc_LIBRARIES ${Tcmalloc_LIBRARY} )
else ()
  set(Tcmalloc_FOUND FALSE)
  set(Tcmalloc_LIBRARIES)
endif ()

if (Tcmalloc_FIND_REQUIRED AND Tcmalloc_LIBRARY_NOT_FOUND)
  message(FATAL_ERROR "NOT Found tcmalloc(${TCMALLOC_NAMES}) library")
else ()
  message(STATUS "Found tcmalloc (library:${Tcmalloc_LIBRARY})")
endif ()

mark_as_advanced(Tcmalloc_LIBRARY)
