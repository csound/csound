# cmake 2.8.7 has a macro GETTEXT_PROCESS_PO_FILES
# We don't use it because it may be too new.
function(compile_po code _infile)
    set(_mofile "${CMAKE_CURRENT_BINARY_DIR}/${code}.mo")
    set(_pofile "${CMAKE_CURRENT_SOURCE_DIR}/${_infile}")
    add_custom_command(
        OUTPUT ${_mofile}
        COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} ARGS -o ${_mofile} ${_pofile}
        DEPENDS ${_pofile}
        )
    INSTALL(FILES ${_mofile}
        DESTINATION ${LOCALE_INSTALL_DIR}/${code}/LC_MESSAGES/
        RENAME "csound6.mo"
        )
    add_custom_target(${code}.mo ALL DEPENDS ${_mofile})
    add_dependencies(translations ${code}.mo)
endfunction(compile_po)

if(USE_GETTEXT)
    ADD_CUSTOM_TARGET(translations ALL echo "Building translations")
    compile_po(en_US american.po)
    compile_po(es_CO es_CO.po)
    compile_po(fr french.po)
    compile_po(de german.po)
    compile_po(it italian.po)
    compile_po(ro romanian.po)
    compile_po(ru russian.po)
endif()

