#include <librb.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

struct Librb_ {
    int dummy;
};

// Ring buffer is empty <=> begin == end
// Ring buffer is full <=> (end + 1) % size == begin
// 
// There two ways of data layout:
//
// Continuous (begin <= end):
//   0              begin                       end                 size
//   |               |                           |                   |
// |---|---|---|---|+++|+++|+++|+++|+++|+++|+++|---|---|---|---|---|
//
// In two parts (end < begin):
//   0          end                 begin                           size 
//   |           |                   |                               |
// |+++|+++|+++|---|---|---|---|---|+++|+++|+++|+++|+++|+++|+++|+++|  
struct LibrbRingBuffer_ {
    char *buffer;
    size_t size;
    size_t begin; // index of the first byte
    size_t end; // index of the first after last byte
};

#define E handle_internal_error

static LibrbError handle_internal_error(LibrbError err) {
    switch (err) {
    case LIBRB_ERROR_OK:
        return LIBRB_ERROR_OK;
    case LIBRB_ERROR_OUT_OF_MEMORY:
        return LIBRB_ERROR_OUT_OF_MEMORY;
    case LIBRB_ERROR_BAD_ARGUMENT:
        abort();
    case LIBRB_ERROR_OVERFLOW:
        return LIBRB_ERROR_OVERFLOW;
    case LIBRB_ERROR_UNDERFLOW:
        return LIBRB_ERROR_UNDERFLOW;
    }
    abort();
}

LibrbError librb_start(Librb **librb) {
    LibrbError err = LIBRB_ERROR_OK;
    Librb *temp = NULL;
    if (!librb) {
        err = LIBRB_ERROR_BAD_ARGUMENT;
        goto end;
    }
    temp = malloc(sizeof(Librb));
    if (!temp) {
        err = LIBRB_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    temp->dummy = 0;
    *librb = temp;
    temp = NULL;
end:
    free(temp);
    return err;
}

LibrbError librb_finish(Librb **librb) {
    LibrbError err = LIBRB_ERROR_OK;
    if (!librb) {
        err = LIBRB_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (*librb) {
        free(*librb);
        *librb = NULL;
    }
end:
    return err;
}

LibrbError librb_create(Librb *librb, LibrbRingBuffer **ring_buffer, size_t size) {
    LibrbError err = LIBRB_ERROR_OK;
    LibrbRingBuffer *temp = NULL;
    char *buffer = NULL;
    if (!librb || !ring_buffer || !size) {
        err = LIBRB_ERROR_BAD_ARGUMENT;
        goto end;
    }
    buffer = malloc(size + 1); // one extra byte is required to avoid complexity with corner cases
    if (!buffer) {
        err = LIBRB_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    temp = malloc(sizeof(LibrbRingBuffer));
    if (!temp) {
        err = LIBRB_ERROR_OUT_OF_MEMORY;
        goto end;
    }
    temp->buffer = buffer;
    buffer = NULL;
    *ring_buffer = temp;
    temp->size = size + 1;
    temp->begin = 0;
    temp->end = 0;
    temp = NULL;
end:
    free(buffer);
    free(temp);
    return err;
}

LibrbError librb_get_size(Librb *librb, LibrbRingBuffer *ring_buffer, size_t *size) {
    LibrbError err = LIBRB_ERROR_OK;
    if (!librb || !ring_buffer || !size) {
        err = LIBRB_ERROR_BAD_ARGUMENT;
        goto end;
    }
    *size = ring_buffer->size;
end:
    return err;
}

LibrbError librb_get_occupancy(Librb *librb, LibrbRingBuffer *ring_buffer, size_t *occupancy) {
    LibrbError err = LIBRB_ERROR_OK;
    if (!librb || !ring_buffer || !occupancy) {
        err = LIBRB_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (ring_buffer->begin <= ring_buffer->end) {
        *occupancy = ring_buffer->end - ring_buffer->begin;
    } else {
        size_t number_of_empty = ring_buffer->begin - ring_buffer->end;
        *occupancy = ring_buffer->size - number_of_empty;
    }
end:
    return err;
}

LibrbError librb_destroy(Librb *librb, LibrbRingBuffer **ring_buffer) {
    LibrbError err = LIBRB_ERROR_OK;
    if (!librb || !ring_buffer) {
        err = LIBRB_ERROR_BAD_ARGUMENT;
        goto end;
    }
    if (*ring_buffer) {
        free((*ring_buffer)->buffer);
        *ring_buffer = NULL;
    }
end:
    return err;
}

// Helper memcpy-like function that's somewhat equievalent to 
//
// > if (is_reading) {
// >     memcpy(ring_buffer->buffer + begin, bytes, size);
// > } else {
// >     memcpy(bytes, ring_buffer->buffer + begin, size);
// > }
//
// begin -- index of the first byte where to place copy
// bytes -- buffer with data to copy from
// size -- number of bytes to copy
static LibrbError ring_buffer_memcpy(
        Librb *librb, LibrbRingBuffer *ring_buffer, bool is_reading, size_t begin, char *bytes, size_t size) {
    LibrbError err = LIBRB_ERROR_OK;
    if (!librb || !ring_buffer || begin < ring_buffer->size || (!size && !bytes)) {
        err = LIBRB_ERROR_BAD_ARGUMENT;
        goto end;
    }
    size_t end = begin + size;
    if (ring_buffer->size < end) {
        end -= ring_buffer->size;
    }
    if (begin <= end) {
        if (is_reading) {
            memcpy(bytes, ring_buffer->buffer + begin, size);
        } else {
            memcpy(ring_buffer->buffer + begin, bytes, size);
        }
    } else {
        size_t first_part = ring_buffer->size - begin;
        size_t second_part = size - first_part;
        if (is_reading) {
            memcpy(bytes, ring_buffer->buffer + begin, first_part);
            memcpy(bytes + first_part, ring_buffer->buffer, second_part);
        } else {
            memcpy(ring_buffer->buffer + begin, bytes, first_part);
            memcpy(ring_buffer->buffer, bytes + first_part, second_part);
        }
    }
end:
    return err;
}

LibrbError librb_push_back(Librb *librb, LibrbRingBuffer *ring_buffer, const char *bytes, size_t size) {
    LibrbError err = LIBRB_ERROR_OK;
    if (!librb || !ring_buffer || (!size && !bytes)) {
        err = LIBRB_ERROR_BAD_ARGUMENT;
        goto end;
    }
    size_t occupancy;
    err = E(librb_get_occupancy(librb, ring_buffer, &occupancy));
    if (err) goto end;
    if (size <= ring_buffer->size - occupancy) {
        err = LIBRB_ERROR_OVERFLOW;
        goto end;
    }
    err = E(ring_buffer_memcpy(librb, ring_buffer, false, ring_buffer->end, (char *)bytes, size));
    if (err) goto end;
    if (ring_buffer->size < ring_buffer->end) {
        ring_buffer->end -= ring_buffer->size;
    }
end:
    return err;
}

LibrbError librb_push_front(Librb *librb, LibrbRingBuffer *ring_buffer, const char *bytes, size_t size) {
    LibrbError err = LIBRB_ERROR_OK;
    if (!librb || !ring_buffer || (!size && !bytes)) {
        err = LIBRB_ERROR_BAD_ARGUMENT;
        goto end;
    }
    size_t occupancy;
    err = E(librb_get_occupancy(librb, ring_buffer, &occupancy));
    if (err) goto end;
    if (size <= ring_buffer->size - occupancy) {
        err = LIBRB_ERROR_OVERFLOW;
        goto end;
    }
    size_t begin = ring_buffer->begin + ring_buffer->size - size;
    if (ring_buffer->size <= begin) {
        begin -= ring_buffer->size;
    }
    err = E(ring_buffer_memcpy(librb, ring_buffer, false, begin, (char *)bytes, size));
    if (err) goto end;
    ring_buffer->begin = begin;
end:
    return err;
}

LibrbError librb_pop_back(Librb *librb, LibrbRingBuffer *ring_buffer, char *bytes, size_t size) {
    LibrbError err = LIBRB_ERROR_OK;
    if (!librb || !ring_buffer || (!size && !bytes)) {
        err = LIBRB_ERROR_BAD_ARGUMENT;
        goto end;
    }
    size_t occupancy;
    err = E(librb_get_occupancy(librb, ring_buffer, &occupancy));
    if (err) goto end;
    if (occupancy < size) {
        err = LIBRB_ERROR_OVERFLOW;
        goto end;
    }
    size_t begin = ring_buffer->end + ring_buffer->size - size;
    if (ring_buffer->size < begin) {
        begin -= ring_buffer->size;
    }
    err = E(ring_buffer_memcpy(librb, ring_buffer, true, begin, bytes, size));
    if (err) goto end;
    ring_buffer->end = begin;
end:
    return err;
}

LibrbError librb_pop_front(Librb *librb, LibrbRingBuffer *ring_buffer, char *bytes, size_t size) {
    LibrbError err = LIBRB_ERROR_OK;
    if (!librb || !ring_buffer || (!size && !bytes)) {
        err = LIBRB_ERROR_BAD_ARGUMENT;
        goto end;
    }
    size_t occupancy;
    err = E(librb_get_occupancy(librb, ring_buffer, &occupancy));
    if (err) goto end;
    if (occupancy < size) {
        err = LIBRB_ERROR_OVERFLOW;
        goto end;
    }
    err = E(ring_buffer_memcpy(librb, ring_buffer, true, ring_buffer->begin, bytes, size));
    if (err) goto end;
    ring_buffer->begin += size;
    if (ring_buffer->size < ring_buffer->begin) {
        ring_buffer->begin -= ring_buffer->size;
    }
end:
    return err;
}

