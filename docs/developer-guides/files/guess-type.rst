Guessing the type of a file
===========================

In order to guess the type of a file, the steps are the following:

1. Open the file using :c:func:`cahute_open_file`.
2. Use :c:func:`cahute_guess_file_type` to obtain the guessed file type.
3. Close the file using :c:func:`cahute_close_file`.

An example program to do this is the following:

.. literalinclude:: guess-type.c
    :language: c
