option(TEST "run tests" OFF)

if (TEST)

aux_source_directory(. Srcs)

enable_testing()

foreach(TestSrc IN LISTS Srcs)
    get_filename_component(TestTarget ${TestSrc} NAME_WE)
    add_executable(${TestTarget} ${TestSrc})
    target_link_libraries(${TestTarget} hgsqlite3w)
    add_test(NAME test_${TestTarget} COMMAND ${TestTarget})
endforeach()

endif()
