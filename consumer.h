/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_CONSUMER_H__
#define __SO_CONSUMER_H__

#include "ring_buffer.h"
#include "packet.h"

#define OPEN_FILE(filename) open((filename), O_RDWR | O_CREAT | O_APPEND, 0666)

typedef struct so_consumer_ctx_t {
	struct so_ring_buffer_t *producer_rb;

	const char *out_filename;
	unsigned long last_time;

	/* TODO: add synchronization primitives for timestamp ordering */
	pthread_mutex_t *log_file;
	pthread_mutex_t *last_timestamp;
} so_consumer_ctx_t;

int create_consumers(pthread_t *tids,
					int num_consumers,
					so_ring_buffer_t *rb,
					const char *out_filename);

#endif /* __SO_CONSUMER_H__ */
