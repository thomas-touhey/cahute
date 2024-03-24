/* Compile using: gcc open-usb-link.c `pkg-config cahute --cflags --libs`. */

#include <stdio.h>
#include <cahute.h>

int main(void) {
    cahute_link *link;
    int err;

    err = cahute_open_simple_usb_link(&link, 0);
    if (err) {
        fprintf(stderr, "cahute_open_usb has returned error 0x%04X.\n", err);
        return 1;
    }

    /* Profit! */
    printf("Link successfully opened!\n");

    cahute_close_link(link);
    return 0;
}
