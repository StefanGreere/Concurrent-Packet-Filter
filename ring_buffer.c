// SPDX-License-Identifier: BSD-3-Clause

#include "ring_buffer.h"
#include <stdlib.h>
#include <pthread.h>

int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
	ring->data = (char *)malloc(cap * sizeof(char));
	if (!ring->data)
		return -1;

	ring->read_pos = 0;
	ring->write_pos = 0;
	ring->len = 0;
	ring->cap = cap;
	ring->access = true; // let the buffer be accessed

	pthread_mutex_init(&ring->locker, NULL);
	pthread_cond_init(&ring->allow_enqueue, NULL);
	pthread_cond_init(&ring->allow_dequeue, NULL);

	return 0;
}

int write_to_ring_buffer(so_ring_buffer_t *ring, void *data, size_t size)
{
	ring->len += size;
	size_t index = ring->write_pos;
	size_t limit = ring->cap;

	while (size && index <= limit) {
		ring->data[index] = *(char *)data;
		index++;
		data++;
		size--;
	}

	// if the buffer is full, write from the beginning (circular buffer)
	int times = size / ring->cap;   // the number of rotations in the buffer

	for (int i = 0; i < times; i++) {
		index = 0;  // start form the ring buffer beginning
		while (size) {
			ring->data[index] = *(char *)data;
			index++;
			data++;
			size--;
		}
	}

	// the final rotation wtrite the remaining data
	while (size) {
		ring->data[index] = *(char *)data;
		index++;
		data++;
		size--;
	}

	return index % ring->cap;  // return the new write position
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
	// it cannot be write if the ring buffer is not accessible
	if (!ring->access)
		return -1;

	// lock the mutex to not be accessed by other threads
	pthread_mutex_lock(&ring->locker);

	// wait until the ring buffer has enough size space to write data
	while ((ring->len + size) > ring->cap)
		pthread_cond_wait(&ring->allow_enqueue, &ring->locker);

	// write to the ring buffer the data and returen the new write position
	int position = write_to_ring_buffer(ring, data, size);

	ring->write_pos = (size_t)position;

	// signal that the buffer could be read
	pthread_cond_signal(&ring->allow_dequeue);

	// unlock the mutex to be accessed by other threads
	pthread_mutex_unlock(&ring->locker);

	return size;
}

int read_from_ring_buffer(so_ring_buffer_t *ring, void *data, size_t size)
{
	ring->len -= size;
	size_t index = ring->read_pos;
	size_t limit = ring->cap;

	while (size && index <= limit) {
		*(char *)data = ring->data[index];
		index++;
		data++;
		size--;
	}

	// if the buffer is full, read from the beginning (circular buffer)
	int times = size / ring->cap;   // the number of rotations in the buffer

	for (int i = 0; i < times; i++) {
		index = 0;  // start form the ring buffer beginning
		while (size) {
			*(char *)data = ring->data[index];
			index++;
			data++;
			size--;
		}
	}

	// the final rotation read the remaining data
	while (size) {
		*(char *)data = ring->data[index];
		index++;
		data++;
		size--;
	}

	return index % ring->cap;  // return the new read position
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	// lock the mutex to not be accessed by other threads
	pthread_mutex_lock(&ring->locker);

	// wait until the ring buffer has enough size data to read
	while (ring->len < size && ring->access)
		pthread_cond_wait(&ring->allow_dequeue, &ring->locker);

	// unlock the mutex if the ring buffer is not accessible
	if (!ring->access && ring->len < size) {
		pthread_mutex_unlock(&ring->locker);
		return -1;
	}

	// read from the ring buffer the data and returen the new read position
	int position = read_from_ring_buffer(ring, data, size);

	ring->read_pos = (size_t)position;

	// signal that the buffer could be written
	pthread_cond_signal(&ring->allow_enqueue);

	// unlock the mutex to be accessed by other threads
	pthread_mutex_unlock(&ring->locker);

	return size;
}

void ring_buffer_destroy(so_ring_buffer_t *ring)
{
	pthread_mutex_destroy(&ring->locker);
	pthread_cond_destroy(&ring->allow_enqueue);
	pthread_cond_destroy(&ring->allow_dequeue);
	free(ring->data);
}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
	ring->access = false;
	pthread_cond_broadcast(&ring->allow_enqueue);
	pthread_cond_broadcast(&ring->allow_dequeue);
}
