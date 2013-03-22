#include "encoding.h"

#include "iconv.h"
#include "err.h"

#include <iconv.h>

struct {
	iconv_t cd;
} g;

int encoding_init() {
	if( (g.cd = iconv_open("UTF8", "WCHAR_T")) == ((iconv_t) -1) )
		return -1;

	return 0;
}

void encoding_free() {
	if(iconv_close(g.cd) != 0)
		err_warn(errno, "failed to close iconv descriptor");
}

size_t encoding_wchar_to_utf8(uint8_t *utf8_data, size_t utf8_len,
		const uint32_t *wchar_data, size_t wchar_len) {
	/* [io]bl => input/output bytes left */
	size_t status, ibl, obl;
	/* input/output pointers */
	char *ip, *op; 

	ip = (char *) wchar_data;
	op = (char *) utf8_data;
	ibl = wchar_len*sizeof(uint32_t);
	obl = utf8_len;

	if( (status = iconv(g.cd, &ip, &ibl, &op, &obl)) == ((size_t) -1) )
		err_panic(errno, "failed to convert data from wchar to utf-8");

	return obl;
}