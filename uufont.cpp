#include "uufont.h"

const uuglyph_t *get_glyph(const uufont_t *font, const uint16_t code) {
  int16_t min = 0, max = font->block_size - 1, mid = 0;
  const uufontblock_t *r = NULL;
  while(min <= max) {
    mid = (max + min) / 2;
    r = &font->blocks[mid];
    const uint16_t first_code = r->first_code;
    const uint16_t glyph_size = r->glyph_size;
    if(code < first_code) {
      max = mid - 1;
    } else if(code >= first_code + glyph_size) {
      min = mid + 1;
    } else {
      return (const uuglyph_t *) &font->glyph_stream[r->glyph[code - first_code]];
    }
  }
  // RETURN FIRST GLYPH
  return (const uuglyph_t *)font->glyph_stream;
}