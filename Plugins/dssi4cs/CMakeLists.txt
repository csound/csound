check_include_file(ladspa.h LADSPA_H)

set(dssi_SRC load.c dssi4cs.c)
make_plugin(dssi4cs "${dssi_SRC}" dl)
if (LADSPA_H)
    target_compile_definitions(dssi4cs PRIVATE "HAVE_LADSPA_H")
endif()
