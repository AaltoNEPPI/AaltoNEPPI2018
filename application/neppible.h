/* Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code is public domain here.
 */
#ifndef NEPPIBLEH
#define NEPPIBLEH

#include "debug.h"
#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

//init
kernel_pid_t neppible_init(void);

//start
void neppible_start(void);

#ifdef __cplusplus
}
#endif

#endif //Include guard
