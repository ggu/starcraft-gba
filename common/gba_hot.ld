/* SPDX-License-Identifier: GPL-3.0-or-later
 * Variant of gba.ld for builds that also run a listed set of ordinary ROM
 * functions from IWRAM. It differs from gba.ld in three ways:
 *
 * - iwram_hot_sections.ld, written by tools/gba_iwram_hot.py from a
 *   config/iwram-hot list, names the .text.<function> sections to place after
 *   the GBA_HOT code. They stay Thumb; the linker adds the long-branch veneers.
 * - .iwram comes before the remaining ROM text, because an input section goes
 *   to the first statement that matches it. Only the header and the reset code
 *   precede it, so the cartridge header stays at 0x08000000.
 * - The IWRAM end is checked against __iwram_hot_limit from
 *   iwram_hot_limit.ld instead of the fixed 0x03005000. That limit is only a
 *   coarse guard: tools/gba_iwram_hot.py check proves, for the linked ROM, that
 *   the worst-case stack still fits above __iwram_end.
 *
 * Link with -Tcommon/gba_hot.ld -L<directory holding the two fragments>. */
ENTRY(_start)
MEMORY {
 ROM (rx) : ORIGIN = 0x08000000, LENGTH = 32M
 EWRAM (rwx) : ORIGIN = 0x02000000, LENGTH = 256K
 IWRAM (rwx) : ORIGIN = 0x03000000, LENGTH = 31K
}
INCLUDE iwram_hot_limit.ld
SECTIONS {
 .text : {
  KEEP(*(.gbaheader))
  *(.text.start)
  . = ALIGN(4);
 } > ROM
 .iwram : {
  __iwram_start = .;
  *(.iwram*)
  __iwram_hot_start = .;
  INCLUDE iwram_hot_sections.ld
  . = ALIGN(4);
  __iwram_end = .;
 } > IWRAM AT > ROM
 __iwram_load = LOADADDR(.iwram);
 .rom : {
  *(.text*)
  KEEP(*(.rodata.gba_save_signature))
  *(.rodata*)
  . = ALIGN(4);
 } > ROM
 .data : {
  __data_start = .;
  *(.data*)
  . = ALIGN(4);
  __data_end = .;
 } > EWRAM AT > ROM
 __data_load = LOADADDR(.data);
 .bss (NOLOAD) : {
  __bss_start = .;
  *(.bss*) *(COMMON)
  . = ALIGN(4);
  __bss_end = .;
 } > EWRAM
 /DISCARD/ : { *(.comment*) *(.note*) *(.ARM.exidx*) *(.ARM.extab*) }
 ASSERT(__iwram_end <= __iwram_hot_limit, "IWRAM code and hot functions exceed the configured limit")
 ASSERT(__bss_end < 0x0203e000, "GBA working RAM exhausted; leave stack/safety margin")
}
