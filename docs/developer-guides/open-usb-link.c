/* Compile using: gcc open-usb-link.c `pkg-config cahute --cflags --libs`. */

#include <stdio.h>
#include <cahute.h>

struct my_detection_cookie {
    int found_bus;
    int found_address;
};

int my_detection_callback(
    struct my_detection_cookie *cookie,
    cahute_usb_detection_entry const *entry
) {
    /* We only want fx-9860G and compatible. */
    if (entry->cahute_usb_detection_entry_type
        != CAHUTE_USB_DETECTION_ENTRY_TYPE_SEVEN)
        return 0; /* Continue. */

    cookie->found_bus = entry->cahute_usb_detection_entry_bus;
    cookie->found_address = entry->cahute_usb_detection_entry_address;
    return 1; /* Interrupt and return CAHUTE_ERROR_INT. */
}

int main(void) {
    struct my_detection_cookie cookie;
    cahute_link *link;
    int err;

    /* This either returns 0 if detection has gotten to the end without finding
     * a suitable device, CAHUTE_ERROR_INT if a suitable device has been
     * found (due to our callback having returned 1), or another error. */
    err = cahute_detect_usb(
        (cahute_detect_usb_entry_func *)&my_detection_callback,
        &cookie
    );
    if (!err) {
        fprintf(stderr, "No USB calculator detected.\n");
        return 1;
    }

    if (err != CAHUTE_ERROR_INT) {
        fprintf(stderr, "cahute_detect_usb has returned error 0x%04X.\n", err);
        return 1;
    }

    err =
        cahute_open_usb_link(&link, 0, cookie.found_bus, cookie.found_address);
    if (err) {
        fprintf(stderr, "cahute_open_usb has returned error 0x%04X.\n", err);
        return 1;
    }

    printf("Link successfully opened!\n");

    cahute_close_link(link);
    return 0;
}
