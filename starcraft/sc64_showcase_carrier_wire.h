/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SHOWCASE_CARRIER_WIRE_H
#define SC64_SHOWCASE_CARRIER_WIRE_H

/* 14,228-byte predecessor (including two Yamato rows), plus 8-byte
 * impact header, 32 x 6-byte impacts and 16-byte transport trailer,
 * leaves 1,924 bytes in the 16,368-byte journal payload. */
#define SC64_SHOWCASE_PROJECTILE_CAP 95u
#define SC64_SHOWCASE_PROJECTILE_BYTES 1922u
#define SC64_SHOWCASE_PROJECTILE_EXTENSION (2u + SC64_SHOWCASE_PROJECTILE_BYTES)

#endif
