/* Compile using: gcc convert-multi-in.c `pkg-config cahute --cflags --libs`. */

#include <stdio.h>
#include <string.h>
#include <cahute.h>

/* Example long buffer. */
static cahute_u8 const example[] =
    "4\x0EN\x0D"
    "8\x0ES\x0D"
    "1\x0E"
    "C\x0D\"ENTREZ LES OPERATEURS\"\x0D"
    "\xF7\x08N\x0D\xF7\x08\x7F\x8F:\xF7\x09:\xF7\x0A:\x7F\x8F\x0E"
    "D\x0D"
    "\xF7\x0B"
    "D=0\x0D\x0D\xF7\x00"
    "D=43:\xF7\x01\x0D\xF7\x10S,4,\"\xA9"
    "\"\x0DS\x89"
    "1\x0ES:N\x99"
    "1\x0EN\x0DN\x11"
    "3\x7F\xB0N\x11"
    "2\x7F\xB0"
    "N\x11"
    "0\x13"
    "0\x0E"
    "C:\xF7\x03\x0D\xF7\x00"
    "D=42:\xF7\x01\x0D"
    "\xF7\x10S,4,\"\x89\"\x0DS\x89"
    "1\x0ES:N\x99"
    "1\x0EN\x0D"
    "0\x0E"
    "C:\xF7\x03\x0D\xF7\x00"
    "D=32:\xF7\x01\x0D\xF7\x10S,4,\"\x99\"\x0DS\x89"
    "1\x0ES:N\x99"
    "1\x0EN\x0DN\x11"
    "1\x13"
    "0\x0E"
    "C:\xF7\x03\x0D\xF7\x00"
    "D=33:\xF7\x01\x0D\xF7\x10S,4,\"\xB9\"\x0DS\x89"
    "1\x0ES:N\x99"
    "1\x0EN"
    "\x0D"
    "0\x0E"
    "C:\xF7\x03\x0D\xF7\x09\x0D\x0D\xF7\x00"
    "C=1:\xF7\x01\x0D"
    "\"GOLDORAK\"\x0D\xF7\x02\x0D\"INVALIDE\"\x0D\xF7\x03\x00";

int main(void) {
    char final_buf[1024];
    cahute_u8 read_buf[32];
    size_t read_offset = 0;
    void *dest = final_buf;
    size_t dest_size = sizeof(final_buf);
    void const *src;
    size_t src_size;
    size_t present = 0;
    int i, err;

    for (i = 0;; i++) {
        size_t read_size;

        /* Start by completing the buffer.
         * If there are ``present`` bytes already in the buffer, we want
         * to add ``sizeof(read_buf) - present`` bytes in the buffer. */
        if (read_offset > sizeof(example))
            break;

        src_size = sizeof(read_buf) - present;

        if (src_size > sizeof(example) - read_offset) {
            /* There may be less bytes to read than expected, we want to
             * complete it this way. */
            src_size = sizeof(example) - read_offset;
        }

        memcpy(&read_buf[present], &example[read_offset], src_size);
        read_offset += src_size;

        /* We now want to incorporate the already-present bytes into the
         * buffer, to prepare for the conversion. */
        src = read_buf;
        src_size += present;
        present = src_size;

        /* We now have an ``src`` buffer of ``src_size`` bytes to read,
         * we can operate the conversion. */
        err = cahute_convert_text(
            &dest,
            &dest_size,
            &src,
            &src_size,
            CAHUTE_TEXT_ENCODING_UTF8,
            CAHUTE_TEXT_ENCODING_9860_8
        );
        printf(
            "Pass %d: %zu bytes read, error set to 0x%04X\n",
            i,
            present - src_size,
            err
        );

        if (err == CAHUTE_ERROR_TERMINATED)
            break; /* A sentinel was found! */

        if (!err) {
            present = 0;
            continue; /* There may be some more bytes to read. */
        }

        if (err == CAHUTE_ERROR_TRUNC) {
            /* Truncated input, we must check that at least one byte has
             * been read from the source data to avoid an infinite loop. */
            if (src_size == present)
                return 1;

            /* Otherwise, we want to copy the leftover bytes at
             * the beginning and complete.
             *
             * NOTE: Both memory areas may overlap, we must use memmove()
             * to avoid overwriting data we're trying to copy! */
            memmove(read_buf, src, src_size);
            present = src_size;
            continue;
        }

        /* Other failure, we must stop! */
        return 1;
    }

    /* Print the result of the conversion. */
    printf("---\n");
    fwrite(final_buf, 1, sizeof(final_buf) - dest_size, stdout);
    printf("\n---\n");
    return 0;
}
