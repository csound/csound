# TODO: add tests for cs
# TODO: add tests for csanalyze
# TODO: add tests for csdebugger
# TODO: add tests for extract
# TODO: add tests for scot
# TODO: add tests for scsort

if (TARGET atsa)
    add_subdirectory(atsa)
endif()

if (TARGET csb64enc)
    add_subdirectory(csb64enc)
endif()

if (TARGET cvanal)
    add_subdirectory(cvanal)
endif()

if (TARGET dnoise)
    add_subdirectory(dnoise)
endif()

if (TARGET envext)
    add_subdirectory(envext)
endif()

if (TARGET extractor)
    add_subdirectory(extractor)
endif()

if (TARGET het_export)
    add_subdirectory(het_export)
endif()

if (TARGET het_import)
    add_subdirectory(het_import)
endif()

if (TARGET hetro)
    add_subdirectory(hetro)
endif()

if (TARGET lpanal)
    add_subdirectory(lpanal)
endif()

if (TARGET lpc_export)
    add_subdirectory(lpc_export)
endif()

if (TARGET lpc_import)
    add_subdirectory(lpc_import)
endif()

if (TARGET makecsd)
    add_subdirectory(makecsd)
endif()

if (TARGET mixer-bin)
    add_subdirectory(mixer_bin)
endif()

if (TARGET pvanal)
    add_subdirectory(pvanal)
endif()

if (TARGET pvlook)
    add_subdirectory(pvlook)
endif()

if (TARGET pv_export)
    add_subdirectory(pv_export)
endif()

if (TARGET pv_import)
    add_subdirectory(pv_import)
endif()

if (TARGET scale)
    add_subdirectory(scale)
endif()

if (TARGET sdif2ad)
    add_subdirectory(sdif2ad)
endif()

if (TARGET sndinfo)
    add_subdirectory(sndinfo)
endif()

if (TARGET srconv)
    add_subdirectory(srconv)
endif()

if (TARGET src_conv)
    add_subdirectory(src_conv)
endif()
