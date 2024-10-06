/* Compile using: gcc guess-type.c `pkg-config cahute --cflags --libs`. */

#include <stdio.h>
#include <cahute.h>

int main(int ac, char **av) {
    cahute_file *file = NULL;
    unsigned long type;
    int err = 0, ret = 0;

    if (ac != 2) {
        fprintf(stderr, "usage: %s <path/to/file.ext>\n", av[0]);
        return 1;
    }

    err = cahute_open_file(&file, 0, av[1], CAHUTE_PATH_TYPE_CLI);
    if (err) {
        fprintf(
            stderr,
            "cahute_open_file() has returned error %s.\n",
            cahute_get_error_name(err)
        );
        return 1;
    }

    err = cahute_guess_file_type(file, &type);
    if (err) {
        ret = 1;
        fprintf(
            stderr,
            "cahute_guess_file_type() has returned error %s.\n",
            cahute_get_error_name(err)
        );
    } else
        printf("Guessed file type: %d\n", type);

    cahute_close_file(file);
    return ret;
}
