/* Compile using: gcc detect-serial.c `pkg-config cahute --cflags --libs`. */

#include <stdio.h>
#include <cahute.h>

int my_callback(void *cookie, cahute_serial_detection_entry const *entry) {
    printf("New entry data:\n");
    printf("- %s\n", entry->cahute_serial_detection_entry_name);

    return 0;
}

int main(void) {
    int err;

    err = cahute_detect_serial(&my_callback, NULL);
    if (err)
        fprintf(stderr, "Cahute has returned error 0x%04X.\n", err);

    return 0;
}
