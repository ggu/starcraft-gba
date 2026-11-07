/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include "../common/gba_flash_transaction.h"
typedef struct {unsigned ime,running,allow,quiesces,restores;} Fake;
static uint16_t read_ime(void *p){return ((Fake*)p)->ime;}
static void write_ime(void *p,uint16_t x){((Fake*)p)->ime=x;}
static int stop(void *p){Fake *f=p;assert(!f->ime);++f->quiesces;if(!f->allow)return 0;f->running=0;return 1;}
static void resume(void *p){Fake *f=p;assert(!f->ime);++f->restores;f->running=1;}
int main(void){Fake f={1,1,1,0,0};GbaFlashTransaction t={&f,read_ime,write_ime,stop,resume,0,0};
 assert(gba_flash_transaction_begin(&t));assert(!f.ime&&!f.running);assert(!gba_flash_transaction_begin(&t));
 gba_flash_transaction_end(&t);assert(f.ime&&f.running&&f.restores==1);gba_flash_transaction_end(&t);assert(f.restores==1);
 f.allow=0;assert(!gba_flash_transaction_begin(&t));assert(f.ime&&f.running&&f.restores==1);
 f.allow=1;f.ime=0;assert(gba_flash_transaction_begin(&t));gba_flash_transaction_end(&t);assert(!f.ime&&f.running&&f.restores==2);
 return 0;}
