target_sources(rsl-log PRIVATE
  terminal.cpp
)

if(UNIX AND NOT APPLE) # Linux only
  find_package(PkgConfig QUIET)

  if(PkgConfig_FOUND)
    pkg_check_modules(SYSTEMD IMPORTED_TARGET libsystemd)
    if(TARGET PkgConfig::SYSTEMD)
      message(STATUS "systemd found: enabling support")
      target_link_libraries(rsl-log PRIVATE PkgConfig::SYSTEMD)
      target_compile_definitions(rsl-log PRIVATE RSL_LOG_SYSTEMD=1)
      target_sources(rsl-log PRIVATE systemd.cpp)
    else()
      message(STATUS "systemd not found: building without systemd support")
    endif()
  endif()
endif()