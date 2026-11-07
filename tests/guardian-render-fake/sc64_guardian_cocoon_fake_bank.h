/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored fake data; no ROM assets. */
static unsigned char fake_primary[4]={3,3,3,3};
static unsigned char fake_shadow[4]={1,1,1,1};
static unsigned char fake_timeline[751],fake_lut[256];
static Sc64CocoonFrame fake_frames[1]={{40,0,2,2,48,2,fake_primary,fake_shadow}};
static Sc64CocoonBank cocoon_bank={fake_frames,1,fake_timeline,1,751,fake_lut,0,1};
