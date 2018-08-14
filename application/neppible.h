/* Copyright Ville Hiltunen 2018 <hiltunenvillej@gmail.com>
 *
 * All code is public domain here.
 */
#ifndef NEPPIBLEH
#define NEPPIBLEH

#include "debug.h"
#include "msg.h"

#define CHANGE_COLOR 777

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int data;
} char_descr_t;

//init
void neppible_init(kernel_pid_t);

//start
void neppible_start(void);

//add char
uint8_t neppible_add_char(uint16_t UUID, char_descr_t descriptions, uint16_t initial_value);

//update
void neppible_update_char(uint16_t UUID, uint16_t new_value);

#ifdef __cplusplus
}
#endif

#endif //Include guard
