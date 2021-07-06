#include <librb.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

int main() {
    LibrbError err = LIBRB_ERROR_OK;
    Librb *librb = NULL;
    LibrbRingBuffer *ring_buffer = NULL;

    err = librb_start(&librb);
    if (err) goto end;

    err = librb_create(librb, &ring_buffer, 3);
    if (err) goto end;

    char str[] = "123";
    err = librb_push_back(librb, ring_buffer, str, strlen(str));
    if (err) goto end;

    char c;
    err = librb_pop_front(librb, ring_buffer, &c, 1);
    if (err) goto end;
    assert(c == '1');

    err = librb_pop_back(librb, ring_buffer, &c, 1);
    if (err) goto end;
    assert(c == '3');

    err = librb_pop_back(librb, ring_buffer, &c, 1);
    if (err) goto end;
    assert(c == '2');

    err = librb_push_front(librb, ring_buffer, str, strlen(str));
    if (err) goto end;

    err = librb_pop_front(librb, ring_buffer, &c, 1);
    if (err) goto end;
    assert(c == '1');

    err = librb_pop_back(librb, ring_buffer, &c, 1);
    if (err) goto end;
    assert(c == '3');

    err = librb_pop_back(librb, ring_buffer, &c, 1);
    if (err) goto end;
    assert(c == '2');

end:
    librb_destroy(librb, &ring_buffer);
    librb_finish(&librb);
    return err;
}
