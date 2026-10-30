/* SPDX-License-Identifier: GPL-3.0-or-later */
static inline unsigned sc64_trace_short_duration_word(int duration)
{ return duration > 0 && duration <= 255 ? (unsigned)duration : 0; }
