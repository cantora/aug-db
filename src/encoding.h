#ifndef AUG_DB_ENCODING_H
#define AUG_DB_ENCODING_H

#include <stdint.h>
#include <stddef.h>

int encoding_init();
void encoding_free();
/* returns the number of bytes left in utf8_data */
size_t encoding_wchar_to_utf8(uint8_t *utf8_data, size_t utf8_len,
		const uint32_t *wchar_data, size_t wchar_len);

#endif /* AUG_DB_ENCODING_H */
