
# Find the native Tcmalloc library
#
#  Tcmalloc_LIBRARIES   - List of libraries when using Tcmalloc.
#  Tcmalloc_FOUND       - True if Tcmalloc found.

if (USE_TCMALLOC)
  set(Tcmalloc_NAMES tcmalloc)
else ()
  set(Tcmalloc_NAMES tcmalloc_minimal tcmalloc)
endif ()

find_library(Tcmalloc_LIBRARY NO_DEFAULT_PATH
  NAMES ${Tcmalloc_NAMES}
  PATHS ${HT_DEPENDENCY_LIB_DIR} /lib /usr/lib /usr/local/lib /opt/local/lib /usr/lib/x86_64-linux-gnu
)

if (Tcmalloc_LIBRARY)
  set(Tcmalloc_FOUND TRUE)
  set(Tcmalloc_LIBRARIES ${Tcmalloc_LIBRARY} )
else ()
  set(Tcmalloc_FOUND FALSE)
  set(Tcmalloc_LIBRARIES)
endif ()

if (Tcmalloc_FOUND)
  message(STATUS "Found Tcmalloc (library:${Tcmalloc_LIBRARY})")
else ()
  if (Tcmalloc_FIND_REQUIRED)
    message(FATAL_ERROR "NOT Found Tcmalloc(${Tcmalloc_NAMES}) library")
  endif ()
endif ()

mark_as_advanced(Tcmalloc_LIBRARY)
