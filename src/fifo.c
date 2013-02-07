#include "fifo.h"
#include "err.h"

#include <string.h>

void fifo_init(struct fifo *f, void *buf, 
		size_t elem_size, size_t n_elem) {
	f->buf = buf;
	f->n = n_elem;
	f->size = elem_size;
	f->write = 0;
	f->read = 0;
}

/* the amount of unconsumed data */
size_t fifo_amt(const struct fifo *f) {
	int amt;
	amt = f->write - f->read;
	if(amt < 0)
		amt += f->n;
	
	if(amt < 0)
		err_panic(0, "didnt expect amt < 0");
	
	return (size_t) amt;
}

size_t fifo_avail(const struct fifo *f) {
	return f->n - fifo_amt(f);
}

#define FIFO_READ_ADDR(_fifo) \
	( (_fifo)->buf + (_fifo)->read*(_fifo)->size )

#define FIFO_WRITE_ADDR(_fifo) \
	( (_fifo)->buf + (_fifo)->write*(_fifo)->size )


size_t fifo_peek(const struct fifo *f, void *dest, size_t n) {
	size_t amt;

	amt = fifo_amt(f);
	n = (n > amt || n < 1)? amt : n;
	if(n < 1)
		return 0;

	memcpy(dest, FIFO_READ_ADDR(f), n*f->size);

	return n;
}

size_t fifo_consume(struct fifo *f, void *dest, size_t n) {
	size_t amt;

	amt = fifo_peek(f, dest, n);
	if(amt > 0)
		f->read = (f->read + amt) % f->n;

	return amt;
}

size_t fifo_write(struct fifo *f, const void *src, size_t n) {
	size_t amt;

	amt = fifo_avail(f);
	n = (n > amt || n < 1)? amt : n;
	if(n < 1)
		return 0;

	memcpy(FIFO_WRITE_ADDR(f), src, n*f->size);
	f->write = (f->write + n) % f->n;
	
	return n;
}

