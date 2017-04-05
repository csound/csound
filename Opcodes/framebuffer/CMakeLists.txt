option(BUILD_FRAMEBUFFER_OPCODES "Build the framebuffer opcodes" ON)


if(BUILD_FRAMEBUFFER_OPCODES)


	set(SOURCES Framebuffer.c OLABuffer.c OpcodeEntries.c)
 	make_plugin(framebuffer "${SOURCES}")

endif()
