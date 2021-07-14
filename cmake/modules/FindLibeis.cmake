find_package(PkgConfig)
pkg_check_modules(PKG_Libeis QUIET libeis)

find_path(Libeis_INCLUDE_DIR
  NAMES libeis.h
  HINTS ${PKG_Libeis_INCLUDE_DIRS}
)
find_library(Libeis_LIBRARY
  NAMES eis
  PATHS ${PKG_Libeis_LIBRARY_DIRS}
)

set(Libeis_VERSION ${PKG_Libeis_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libeis
  FOUND_VAR Libeis_FOUND
  REQUIRED_VARS
    Libeis_LIBRARY
    Libeis_INCLUDE_DIR
  VERSION_VAR Libeis_VERSION
)

if(Libeis_FOUND AND NOT TARGET Libeis::Libeis)
  add_library(Libeis::Libeis UNKNOWN IMPORTED)
  set_target_properties(Libeis::Libeis PROPERTIES
    IMPORTED_LOCATION "${Libeis_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PKG_Libeis_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${Libeis_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(Libeis_INCLUDE_DIR Libeis_LIBRARY)
