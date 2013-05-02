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

/* do safety checks */
#define AUG_DB_FIFO_SAFE

/* the amount of unconsumed data */
size_t fifo_amt(const struct fifo *f) {

#ifdef AUG_DB_FIFO_SAFE
	if(f->data_len > f->n)
		err_panic(0, "didnt expect data_len to exceed n");
#endif

	return f->data_len;
}

size_t fifo_avail(const struct fifo *f) {
	return f->n - fifo_amt(f);
}

#define AUG_DB_FIFO_OVERFLOW(start, offset, n) \
	( (start + offset) % n )

/* n must be less than or equal to fifo_amt(f). 
 */
void fifo_peek(const struct fifo *f, void *dest, size_t n) {
	size_t amt, overflow;
	void *read_ptr;
	
	if(n < 1)
		return;

#ifdef AUG_DB_FIFO_SAFE
	if(n > f->n)
		err_panic(0, "expected n <= f->n");
#endif

	overflow = AUG_DB_FIFO_OVERFLOW(f->read, n, f->n);
	read_ptr = f->buf + f->read*f->size;
	
	if(overflow <= f->read) {
		/* we overflowed into earlier part of the buffer */
#ifdef AUG_DB_FIFO_SAFE
		if(f->read >= f->n)
			err_panic(0, "expected f->read < f-n");
#endif
		amt = (f->n - f->read)*f->size;
		memcpy(dest, read_ptr, amt);
		memcpy(dest+amt, f->buf, overflow*f->size);
	}
	else
		memcpy(dest, read_ptr, n*f->size);
}

/* n must be less than or equal to fifo_amt(f). 
 */
void fifo_consume(struct fifo *f, void *dest, size_t n) {
	fifo_peek(f, dest, n);
	f->read = (f->read + n) % f->n;
#ifdef AUG_DB_FIFO_SAFE
	if(n > f->data_len)
		err_panic(0, "expected f->data_len < n");
#endif
	f->data_len -= n;
}

/* n must be less than or equal to 
 * fifo_avail(f).
 */
void fifo_write(struct fifo *f, const void *src, size_t n) {
	size_t overflow, start, amt;
	void *write;

	if(n < 1)
		return;

#ifdef AUG_DB_FIFO_SAFE
	if(n > fifo_avail(f))
		err_panic(0, "expected fifo_avail(f) < n");
#endif

	start = (f->read + f->data_len)%f->n;
	overflow = AUG_DB_FIFO_OVERFLOW(start, n, f->n);
	write = (f->buf + start*f->size);

	if(overflow <= start) {
		/* our write overflows the buffer bound */
		amt = (f->n - start)*f->size;
		memcpy(write, src, amt);
		memcpy(f->buf, src + amt, overflow * f->size);
	}
	else
		memcpy(write, src, n * f->size);

	f->data_len += n;
}

