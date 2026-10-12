# Field screen callback checks

These are bounded comparisons against the original NFL Street module (DOL SHA-1
`46e3de854998a0f66c9be8c72a1164d22cd29eb4`). They exercise callback routines
directly on saved RAM contexts 14, 15, and 16. They are not a complete field
screen startup or visible UI integration result.

The isolated checker is
[`field_callback_original_check.c`](field_callback_original_check.c), built by
[`build_windows_field_callback_original_check.sh`](build_windows_field_callback_original_check.sh).
It runs untouched original class callbacks and supplies only the font resource
lookup and glyph-width calls used by the width case.

## Verified cases

- Class 0 property 0 with arguments `{0, 0xC3080000, 0, 0}` updates the 76-byte
  object exactly. The saved DOL scale is 16; the three scaled shorts are written
  at offsets `+52`, `+54`, and `+56`, and the original callback sets flag
  `0x4000`.
- Class 8 property 5 preserves the object except for its status bits. Raw
  argument words `0`, `0x3f800000`, `0xbf800000`, and `0xbf000000` produce low
  status bits `1`, `2`, `0`, and `0` respectively.
- Class 8 property 12 was checked on the constructed entry 8 header objects at
  `0x81104CC0` and `0x81104D60` (font 0). The original callback's font lookup
  and 12 glyph-width calls were supplied from the parsed font asset. For both
  objects, argument word `0x41300000` and raw word `11` return width `110`.

Property 12 evidence covers these two constructed, single-line objects and the
supplied glyph-width boundary. It does not establish behavior for arbitrary
text, multiline objects, or unsupplied font/resource I/O.
