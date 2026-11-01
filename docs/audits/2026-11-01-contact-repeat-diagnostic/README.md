# Consecutive contact geometry diagnostic

`SC64_DIAGNOSTIC_CONTACT_REPEAT` is disabled in normal profiles. It adds four
cumulative `u32` counters, reset by ordinary process initialization and read as
modulo-2^32 differences: uncached mask calls (including early returns), validated
geometry applications, applications with nonzero offset, and those nonzero
applications whose effective radius and offset match the preceding geometry
application within the same uncached query. Speed is constant within a query.
Memo hits are excluded; fallback and filtered candidates do not invoke geometry
and do not reset the previous geometry key. Zero-offset applications remain in
the sequence when speed is zero, but are not counted as nonzero repeats.

The preceding key lives only on the query's stack. Existing collision checks,
geometry applications, mask accumulation, memo behavior, and lifecycle hooks
are unchanged. Counter helpers reside in ROM and add instrumentation cost;
no diagnostic timing is production timing. Source-only preparation is not
measurement, proof of memory fit, or an optimization result. Normal OFF ROM
identity, the existing memory guards and a source-matched physical replay must
be checked before using the counters.
