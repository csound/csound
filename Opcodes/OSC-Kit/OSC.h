/*
 * $Id$
 *
 * This is to provide a decent wrapper to the OSC header files,
 * at least on a temporary basis... [nicb@axnet.it]
 *
 * This is supposed to go in <local-include>/OSC, along with the other
 * OSC header files
 */
#if !defined(_OSC_OSC_h_)
#	define _OSC_OSC_h_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if defined(TRUE)
#	undef TRUE		/* otherwise we will lose some typedefing */
#endif  /* defined(TRUE) */
#if defined(FALSE)
#	undef FALSE		/* same as above */
#endif /* defined(FALSE) */

#include "OSC-Kit/OSC-common.h"

#include "OSC-Kit/OSC-timetag.h"
#include "OSC-Kit/OSC-address-space.h"
#include "OSC-Kit/OSC-receive.h"
#include "OSC-Kit/NetworkUDP.h"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !defined(_OSC_OSC_h_) */
