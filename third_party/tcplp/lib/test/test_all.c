#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <openthread/message.h>
#include <openthread/tcp.h>

#include "../bitmap.h"
#include "../cbuf.h"

uint32_t num_tests_passed = 0;
uint32_t num_tests_failed = 0;

uint16_t otMessageRead(const otMessage *aMessage, uint16_t aOffset, void *aBuf, uint16_t aLength) {
    return aLength;
}

int otMessageWrite(otMessage *aMessage, uint16_t aOffset, const void *aBuf, uint16_t aLength) {
    return aLength;
}

void bmp_print(uint8_t* buf, size_t buflen) {
    size_t i;
    for (i = 0; i < buflen; i++) {
        printf("%02X", buf[i]);
    }
    printf("\n");
}

void bmp_test(const char* test_name, uint8_t* buf, size_t buflen, const char* contents) {
    char buf_string[(buflen << 1) + 1];
    buf_string[0] = '\0';
    for (size_t i = 0; i < buflen; i++) {
        snprintf(&buf_string[i << 1], 3, "%02X", buf[i]);
    }
    if (strcmp(contents, buf_string) == 0) {
        printf("%s: PASS\n", test_name);
        num_tests_passed++;
    } else {
        printf("%s: FAIL: %s vs. %s\n", test_name, contents, buf_string);
        num_tests_failed++;
    }
}

void test_bmp() {
    size_t test_bmp_size = 8;
    uint8_t buffer[test_bmp_size];

    bmp_init(buffer, test_bmp_size);
    bmp_test("bmp_init", buffer, test_bmp_size, "0000000000000000");

    bmp_setrange(buffer, 11, 7);
    bmp_test("bmp_setrange 1", buffer, test_bmp_size, "001FC00000000000");

    bmp_setrange(buffer, 35, 3);
    bmp_test("bmp_setrange 2", buffer, test_bmp_size, "001FC0001C000000");

    bmp_setrange(buffer, 47, 4);
    bmp_test("bmp_setrange 3", buffer, test_bmp_size, "001FC0001C01E000");

    bmp_swap(buffer, 3, 36, 1);
    bmp_test("bmp_swap 1", buffer, test_bmp_size, "101FC0001401E000");

    bmp_swap(buffer, 0, 40, 24);
    bmp_test("bmp_swap 2", buffer, test_bmp_size, "01E0000014101FC0");

    bmp_swap(buffer, 2, 42, 15);
    bmp_test("bmp_swap 3", buffer, test_bmp_size, "101F80001401E040");

    bmp_swap(buffer, 13, 23, 2);
    bmp_test("bmp_swap 4", buffer, test_bmp_size, "101981801401E040");

    bmp_swap(buffer, 0, 35, 24);
    bmp_test("bmp_swap 5", buffer, test_bmp_size, "A00F028002033020");
}

void cbuf_test(const char* test_name, struct cbufhead* chdr, const char* contents) {
    char buf_string[chdr->size + 1];
    struct otLinkedBuffer first;
    struct otLinkedBuffer second;
    cbuf_reference(chdr, &first, &second);


    memcpy(&buf_string[0], &first.mData[0], first.mLength);
    if (first.mNext != NULL) {
        assert(first.mNext == &second);
        memcpy(&buf_string[first.mLength], &second.mData[0], second.mLength);
        assert(second.mNext == NULL);
        buf_string[first.mLength + second.mLength] = '\0';
    } else {
        buf_string[first.mLength] = '\0';
    }

    if (strcmp(contents, buf_string) == 0) {
        printf("%s: PASS\n", test_name);
        num_tests_passed++;
    } else {
        printf("%s: FAIL: %s (%zu) vs. %s (%zu)\n", test_name, contents, strlen(contents), buf_string, strlen(buf_string));
        num_tests_failed++;
    }
}

void cbuf_write_string(struct cbufhead* chdr, const char* string) {
    cbuf_write(chdr, string, 0, strlen(string), cbuf_copy_into_buffer);
}

void test_cbuf() {
    uint8_t buffer[65];
    uint8_t bitmap[8];
    struct cbufhead chdr;

    cbuf_init(&chdr, buffer, 64); // capacity is actually 64
    cbuf_test("cbuf_init", &chdr, "");

    cbuf_write_string(&chdr, "abcdefghijklmnopqrstuvwxyz0123456789");
    cbuf_test("cbuf_write", &chdr, "abcdefghijklmnopqrstuvwxyz0123456789");

    cbuf_pop(&chdr, 1);
    cbuf_test("cbuf_pop", &chdr, "bcdefghijklmnopqrstuvwxyz0123456789");

    cbuf_pop(&chdr, 5);
    cbuf_test("cbuf_pop", &chdr, "ghijklmnopqrstuvwxyz0123456789");

    cbuf_write_string(&chdr, "abcdefghijklmnopqrstuvwxyz01234567");
    cbuf_test("cbuf_write", &chdr, "ghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz01234567");

    cbuf_contiguify(&chdr, NULL);
    cbuf_test("cbuf_contiguify", &chdr, "ghijklmnopqrstuvwxyz0123456789abcdefghijklmnopqrstuvwxyz01234567");

    cbuf_pop(&chdr, 50);
    cbuf_test("cbuf_pop", &chdr, "uvwxyz01234567");

    cbuf_write_string(&chdr, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"); // yz overflows and isn't written
    cbuf_test("cbuf_write", &chdr, "uvwxyz01234567ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx");

    cbuf_contiguify(&chdr, NULL);
    cbuf_test("cbuf_contiguify", &chdr, "uvwxyz01234567ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx");

    cbuf_contiguify(&chdr, NULL); // check that a second "contiguify" operation doesn't mess things up
    cbuf_test("cbuf_contiguify", &chdr, "uvwxyz01234567ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx");

    cbuf_pop(&chdr, 20);
    cbuf_test("cbuf_pop", &chdr, "GHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx");

    cbuf_write_string(&chdr, "yz");
    cbuf_test("cbuf_write", &chdr, "GHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

    bmp_init(bitmap, 8);
    bmp_test("bmp_init", bitmap, 8, "0000000000000000");

    cbuf_reass_write(&chdr, 4, "@@@@@@@@@@@", 0, 11, bitmap, NULL, cbuf_copy_from_buffer);
    cbuf_test("cbuf_reass_write (cbuf)", &chdr, "GHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    bmp_test("cbuf_reass_write (bitmap)", bitmap, 8, "03FF800000000000");

    cbuf_contiguify(&chdr, bitmap);
    cbuf_test("cbuf_contiguify (cbuf)", &chdr, "GHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    bmp_test("cbuf_reass_write (bitmap)", bitmap, 8, "0000000000003FF8");

    cbuf_write_string(&chdr, "1234");
    cbuf_test("cbuf_write", &chdr, "GHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234");

    cbuf_reass_merge(&chdr, 9, bitmap);
    cbuf_test("cbuf_reass_merge (cbuf)", &chdr, "GHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234@@@@@@@@@");
    bmp_test("cbuf_reass_merge (bitmap)", bitmap, 8, "0000000000000018");

    cbuf_reass_merge(&chdr, 2, bitmap);
    cbuf_test("cbuf_reass_merge (cbuf)", &chdr, "GHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234@@@@@@@@@@@");
    bmp_test("cbuf_reass_merge (bitmap)", bitmap, 8, "0000000000000000");

    cbuf_pop(&chdr, 61);
    cbuf_test("cbuf_pop", &chdr, "");
}

void test_cbuf_2() {
    uint8_t buffer[32];
    uint8_t bitmap[4];
    struct cbufhead chdr;

    cbuf_init(&chdr, buffer, 32);
    cbuf_test("cbuf_init", &chdr, "");

    bmp_init(bitmap, 4);
    bmp_test("bmp_init", bitmap, 4, "00000000");

    cbuf_reass_write(&chdr, 6, "abcdefghijklmnopqrstuvwxyz", 0, 26, bitmap, NULL, cbuf_copy_from_buffer);
    cbuf_test("cbuf_reass_write (cbuf)", &chdr, "");
    bmp_test("cbuf_reass_write (bitmap)", bitmap, 4, "03FFFFFF");

    cbuf_write_string(&chdr, "ASDFGH");
    cbuf_test("cbuf_write (cbuf)", &chdr, "ASDFGH");
    bmp_test("cbuf_write (bitmap)", bitmap, 4, "03FFFFFF");

    cbuf_pop(&chdr, 6);
    cbuf_test("cbuf_pop (cbuf)", &chdr, "");
    bmp_test("cbuf_pop (bitmap)", bitmap, 4, "03FFFFFF");

    cbuf_reass_write(&chdr, 26, "!@#$^&", 0, 6, bitmap, NULL, cbuf_copy_from_buffer);
    cbuf_test("cbuf_reass_write (cbuf)", &chdr, "");
    bmp_test("cbuf_reass_write (bitmap)", bitmap, 4, "FFFFFFFF");

    printf("Count Set: %d (should be at least 32)\n", (int) cbuf_reass_count_set(&chdr, 0, bitmap, 32));

    cbuf_reass_merge(&chdr, 32, bitmap);
    cbuf_test("cbuf_reass_merge (cbuf)", &chdr, "abcdefghijklmnopqrstuvwxyz!@#$^&");
    bmp_test("cbuf_reass_merge (bitmap)", bitmap, 4, "00000000");
}

int main(int argc, char** argv) {
    test_bmp();
    test_cbuf();
    test_cbuf_2();

    printf("%" PRIu32 " tests passed (out of %" PRIu32 ")\n", num_tests_passed, num_tests_passed + num_tests_failed);
    if (num_tests_failed != 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
