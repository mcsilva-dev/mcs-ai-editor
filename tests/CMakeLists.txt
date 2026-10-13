# mcs_add_test(<nome> <bibliotecas...>): cria executável de teste Qt Test e registra no CTest.
function(mcs_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE Qt6::Test mcs_warnings ${ARGN})
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    if(WIN32)
        set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}")
        # windeployqt do app copia Qt6Core*.dll para a mesma pasta dos testes, e esse Qt
        # procura plugins ao lado de si (só qwindows). Aponta para os plugins da instalação.
        set_property(TEST ${name} APPEND PROPERTY ENVIRONMENT
            "QT_PLUGIN_PATH=${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS}")
    endif()
endfunction()
mcs_add_test(tst_toolchain mcs_media)
mcs_add_test(tst_mainwindow mcs_ui)
