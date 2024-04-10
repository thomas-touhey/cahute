/* Compile using: gcc convert-multi-out.c `pkg-config cahute --cflags --libs`. */

#include <stdio.h>
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
    cahute_u8 buf[64];
    void const *src = example;
    size_t src_size = sizeof(example);
    void *dest;
    size_t dest_size;
    int i, err;

    for (i = 0;; i++) {
        size_t converted;

        dest = buf;
        dest_size = sizeof(buf);

        err = cahute_convert_text(
            &dest,
            &dest_size,
            &src,
            &src_size,
            CAHUTE_TEXT_ENCODING_UTF8,
            CAHUTE_TEXT_ENCODING_9860_8
        );
        converted = sizeof(buf) - dest_size;

        printf(
            "Pass %d: %zu bytes converted, error set to 0x%04X:\n",
            i,
            converted,
            err
        );
        if (converted) {
            printf("---\n");
            fwrite(buf, 1, converted, stdout);
            printf("\n---\n");
        }

        if (err == CAHUTE_ERROR_SIZE) {
            /* Not enough bytes in the destination buffer.
             * We want to check that at least one byte has been converted,
             * otherwise it means our buffer is not big enough for the
             * first byte. */
            if (!converted)
                break;

            continue;
        }

        break;
    }

    if (err && err != CAHUTE_ERROR_TERMINATED)
        printf("Conversion has failed.\n");

    return 0;
}
