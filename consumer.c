// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

// the mutex used for the output file
pthread_mutex_t log_file;
// the mutex used for the flag that helps in packets ordering
pthread_mutex_t last_timestamp;

void initialize_mutexes(void)
{
	pthread_mutex_init(&log_file, NULL);
	pthread_mutex_init(&last_timestamp, NULL);
}

int status_packet(ssize_t size, so_consumer_ctx_t *ctx)
{
	if (size <= 0) {
		// protect the access to the important variables
		pthread_mutex_lock(ctx->last_timestamp);
		pthread_mutex_lock(ctx->log_file);

		// if the producer has stopped, break the loop
		if (!ctx->producer_rb->access) {
			pthread_mutex_unlock(ctx->last_timestamp);
			pthread_mutex_unlock(ctx->log_file);
			return 0;
		}
		pthread_mutex_unlock(ctx->last_timestamp);
		pthread_mutex_unlock(ctx->log_file);

		// if the ring buffer is accessible, continue
		return 1;
	}

	// if the packet is valid, process it
	return 2;
}

void write_into_file(int out_file, so_packet_t *pkt, pthread_mutex_t *log_file)
{
	so_action_t action = process_packet(pkt);
	unsigned long hash = packet_hash(pkt);

	char log_packet[PKT_SZ];
	int len = snprintf(log_packet, PKT_SZ, "%s %016lx %lu\n", RES_TO_STR(action), hash, pkt->hdr.timestamp);

	pthread_mutex_lock(log_file);
	write(out_file, log_packet, len);
	pthread_mutex_unlock(log_file);
}

void manage_packet(ssize_t size, char *buffer, so_consumer_ctx_t *ctx, int out_file)
{
	if (size > 0) {
		struct so_packet_t *pkt = (struct so_packet_t *)buffer;

		write_into_file(out_file, pkt, ctx->log_file);
	}
}

void consumer_thread(so_consumer_ctx_t *ctx)
{
	(void) ctx;

	int out_file = OPEN_FILE(ctx->out_filename);
	char buffer[PKT_SZ];

	if (out_file < 0)
		return;

	while (1) {
		// take a packet from the ring buffer
		ssize_t size = ring_buffer_dequeue(ctx->producer_rb, buffer, PKT_SZ);

		int ret = status_packet(size, ctx);

		// if the producer has stopped, break the loop
		if (ret == 0)
			break;
		else if (ret == 1)
			continue;

		// ret = 2 so the packet can be processed and written into the file
		manage_packet(size, buffer, ctx, out_file);
	}

	close(out_file);
}

void update_consumer(so_consumer_ctx_t *ctx, so_ring_buffer_t *rb, const char *out_filename)
{
	ctx->producer_rb = rb;
	ctx->out_filename = out_filename;
	ctx->last_time = 0;

	ctx->log_file = &log_file;
	ctx->last_timestamp = &last_timestamp;
}

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
	(void) tids;
	(void) num_consumers;
	(void) rb;
	(void) out_filename;

	initialize_mutexes();

	so_consumer_ctx_t *ctx_array = calloc(num_consumers, sizeof(so_consumer_ctx_t));

	if (!ctx_array)
		return -1;

	// for each consumer, update its data and then create the thread
	for (int i = 0; i < num_consumers; i++) {
		update_consumer(&ctx_array[i], rb, out_filename);
		pthread_create(&tids[i], NULL, (void *)consumer_thread, &ctx_array[i]);
	}

	return num_consumers;
}
