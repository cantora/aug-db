#ifndef AUG_DB_FIFO_H
#define AUG_DB_FIFO_H

#include <stddef.h>

struct fifo {
	void *buf;
	size_t n;
	size_t size;	
	size_t write;
	size_t read;
};

void fifo_init(struct fifo *f, void *buf, 
		size_t elem_size, size_t n_elem);

size_t fifo_amt(const struct fifo *f);
size_t fifo_avail(const struct fifo *f);

size_t fifo_peek(const struct fifo *f, void *dest, size_t n);
#define fifo_top(_f_ptr, _dest) \
	fifo_peek(_f_ptr, _dest, 1)

size_t fifo_consume(struct fifo *f, void *dest, size_t n);
#define fifo_pop(_f_ptr, _dest) \
	fifo_consume(_f_ptr, _dest, 1)

size_t fifo_write(struct fifo *f, const void *src, size_t n);
#define fifo_push(_f_ptr, _src) \
	fifo_write(_f_ptr, _src, 1)
	
#endif /* AUG_DB_FIFO_H */
