#ifndef LIBRB_H
#define LIBRB_H

#include <stddef.h>

typedef struct Librb_ Librb;

typedef struct LibrbRingBuffer_ LibrbRingBuffer;

typedef enum {
    LIBRB_ERROR_OK,
    LIBRB_ERROR_OUT_OF_MEMORY,
    LIBRB_ERROR_BAD_ARGUMENT,
    LIBRB_ERROR_OVERFLOW,
    LIBRB_ERROR_UNDERFLOW,
} LibrbError;

// Start working with the library. Create instance of Librb.
LibrbError librb_start(Librb **librb);

// Finish working with the library. Free instance of Librb.
LibrbError librb_finish(Librb **librb);

// Create a ring buffer of specified size.
// Precondition: size > 0
LibrbError librb_create(Librb *librb, LibrbRingBuffer **ring_buffer, size_t size);

// Get size of the internal buffer of ring buffer.
LibrbError librb_get_size(Librb *librb, LibrbRingBuffer *ring_buffer, size_t *size);

// Get number of bytes currently in the ring buffer.
LibrbError librb_get_occupancy(Librb *librb, LibrbRingBuffer *ring_buffer, size_t *occupancy);

// Free instance of ring buffer.
LibrbError librb_destroy(Librb *librb, LibrbRingBuffer **ring_buffer);

// Add bytes to the end of the ring buffer.
// Precondition: !size || bytes
LibrbError librb_push_back(Librb *librb, LibrbRingBuffer *ring_buffer, const char *bytes, size_t size);

// Add bytes to the beginning of the ring buffer.
// Precondition: !size || bytes
LibrbError librb_push_front(Librb *librb, LibrbRingBuffer *ring_buffer, const char *bytes, size_t size);

// Remove bytes from the end of the ring buffer.
// Precondition: !size || bytes
LibrbError librb_pop_back(Librb *librb, LibrbRingBuffer *ring_buffer, char *bytes, size_t size);

// Remove bytes from the beginning of the ring buffer.
// Precondition: !size || bytes
LibrbError librb_pop_front(Librb *librb, LibrbRingBuffer *ring_buffer, char *bytes, size_t size);

#endif
