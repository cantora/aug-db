#include "fifo.h"
#include "err.h"

#include <string.h>

void fifo_init(struct fifo *f, void *buf, 
		size_t elem_size, size_t n_elem) {
	f->buf = buf;
	f->n = n_elem;
	f->size = elem_size;
	f->data_len = 0;
	f->read = 0;
}

/* the amount of unconsumed data */
size_t fifo_amt(const struct fifo *f) {
	if(f->data_len > f->n)
		err_panic(0, "didnt expect data_len to exceed n");

	return f->data_len;
}

size_t fifo_avail(const struct fifo *f) {
	return f->n - fifo_amt(f);
}

#define FIFO_READ_ADDR(_fifo) \
	( (_fifo)->buf + (_fifo)->read*(_fifo)->size )

#define FIFO_WRITE_ADDR(_fifo) \
	( (_fifo)->buf + ( ((_fifo)->read+(_fifo)->data_len)%(_fifo)->n )*(_fifo)->size )

/* n must be less than or equal to fifo_amt(f). 
 */
void fifo_peek(const struct fifo *f, void *dest, size_t n) {
	memcpy(dest, FIFO_READ_ADDR(f), n*f->size);
}

/* n must be less than or equal to fifo_amt(f). 
 */
void fifo_consume(struct fifo *f, void *dest, size_t n) {
	fifo_peek(f, dest, n);
	f->read = (f->read + n) % f->n;
	f->data_len -= n;
}

/* n must be less than or equal to 
 * fifo_avail(f).
 */
void fifo_write(struct fifo *f, const void *src, size_t n) {
	memcpy(FIFO_WRITE_ADDR(f), src, n*f->size);
	f->data_len += n;
}

