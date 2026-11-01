# Z7 Transport1 save fixtures

The Z7 candidate tests exercise the current Transport1 namespace by retaining the impact and transport suffix of an existing ordinary save when they replace its RaceSave prefix. The test writer is host-only; non-Transport1 test profiles keep their existing envelope. The positive fixture creates paid Eggs, saves and loads through the ordinary runtime, checks the current envelope and resumes cancellation and paired births. No gameplay or save-format code changes in this branch.

The same fixture changes passed on source `4fc32f5592ac00ae0f38f4cfbb3493ed87187809` with a separate shared-Egg runtime experiment; that is not evidence for this branch's unchanged runtime. Matching tests and the clean default suite for this branch remain pending. The legacy malformed-candidate tests do not establish compatibility with guest SRAM or other save namespaces.
