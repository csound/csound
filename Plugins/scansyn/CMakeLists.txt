set(scansyn_SRCS scansyn.c scansynx.c)
make_plugin(scansyn "${scansyn_SRCS}" ${MATH_LIBRARY})
