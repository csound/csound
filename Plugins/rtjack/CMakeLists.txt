set(rtjack_SRCS rtjack.c alphanumcmp.c)
make_plugin(rtjack "${rtjack_SRCS}" Jack::jack libcsound_internals)