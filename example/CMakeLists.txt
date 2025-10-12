project(rsl-log)

function(DEFINE_EXAMPLE TARGET)
  add_executable(example_${TARGET} "${TARGET}.cpp")
  target_link_libraries(example_${TARGET} PRIVATE rsl-log)
endfunction()

# DEFINE_EXAMPLE(args)
# DEFINE_EXAMPLE(colorize)
DEFINE_EXAMPLE(spans)
# DEFINE_EXAMPLE(coroutine)
DEFINE_EXAMPLE(systemd)