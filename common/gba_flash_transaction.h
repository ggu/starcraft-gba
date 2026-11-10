/* SPDX-License-Identifier: GPL-3.0-or-later
 * Explicitly scoped transaction gate. Hooks run with IME disabled. */
#ifndef GBA_FLASH_TRANSACTION_H
#define GBA_FLASH_TRANSACTION_H
#include <stdint.h>
typedef struct {
 void *context;
 uint16_t (*read_ime)(void *);
 void (*write_ime)(void *,uint16_t);
 int (*quiesce)(void *); /* on failure must leave audio/timer unchanged */
 void (*restore)(void *);
 uint16_t saved_ime;
 unsigned active;
} GbaFlashTransaction;
static int gba_flash_transaction_begin(GbaFlashTransaction *t) {
 if(!t || t->active || !t->read_ime || !t->write_ime || !t->quiesce || !t->restore)return 0;
 t->saved_ime=t->read_ime(t->context);t->write_ime(t->context,0);
 if(!t->quiesce(t->context)){t->write_ime(t->context,t->saved_ime);return 0;}
 t->active=1;return 1;
}
static void gba_flash_transaction_end(GbaFlashTransaction *t) {
 if(!t || !t->active)return;
 t->restore(t->context);t->active=0;t->write_ime(t->context,t->saved_ime);
}
#endif
