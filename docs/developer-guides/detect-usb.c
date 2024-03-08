/* Compile using: gcc detect-usb.c `pkg-config cahute --cflags --libs`. */

#include <stdio.h>
#include <cahute.h>

int my_callback(void *cookie, cahute_usb_detection_entry const *entry) {
    char const *type_name;

    switch (entry->cahute_usb_detection_entry_type) {
    case CAHUTE_USB_DETECTION_ENTRY_TYPE_SEVEN:
        type_name = "fx-9860G or compatible";
        break;

    case CAHUTE_USB_DETECTION_ENTRY_TYPE_SCSI:
        type_name = "fx-CG or compatible";
        break;

    default:
        type_name = "(unknown)";
    }

    printf("New entry data:\n");
    printf(
        "- Address: %03d:%03d\n",
        entry->cahute_usb_detection_entry_bus,
        entry->cahute_usb_detection_entry_address
    );
    printf("- Type: %s\n", type_name);

    return 0;
}

int main(void) {
    int err;

    err = cahute_detect_usb(&my_callback, NULL);
    if (err)
        fprintf(stderr, "Cahute has returned error 0x%04X.\n", err);

    return 0;
}
