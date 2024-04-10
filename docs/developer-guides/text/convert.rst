.. _guide-developer-convert-text:

Converting text from an encoding to another
===========================================

In order to convert text from a text encoding to another, you must use
the :c:func:`cahute_convert_text` function. There are several possible
approaches you can take.

Using single pass conversion for small buffers
----------------------------------------------

For small blobs of data, such as the name of a program, or the name of a file
or directory, you can use a one pass approach with a static buffer.

In this case, you can make a single call to :c:func:`cahute_convert_text`,
which in nominal cases, should return either:

* :c:macro:`CAHUTE_OK`, if the input data has been read in its entirety,
  and no sentinels were detected.
* :c:macro:`CAHUTE_ERROR_TERMINATED`, if a sentinel has been encountered
  in the source data.

An example implementation is the following:

.. literalinclude:: convert-single.c
    :language: c

This program displays the following output:

.. code-block:: text

    Result: \\fls0\angθ.txt

Using multi pass conversion on output
-------------------------------------

If your source data is larger, you can do multiple passes into a buffer
before either placing the result into a stream, or reallocating a buffer
progressively using ``realloc()``.

For every pass, you need to call :c:func:`cahute_convert_text` with the
output set to your buffer, and the input set to your source memory.
On every pass, in nominal circumstances, the function will return one of
the following:

* :c:macro:`CAHUTE_OK`, if the conversion has terminated successfully,
  i.e. if there was no more contents to read from the input.
* :c:macro:`CAHUTE_ERROR_TERMINATED`, if the conversion has been interrupted
  due to a sentinel being found in the source data.
* :c:macro:`CAHUTE_ERROR_SIZE`, if the conversion has run out of space
  in the output buffer, prompting you to make another pass after reading
  the contents of the output buffer.

An example that places the result of each pass into the standard output is
the following:

.. literalinclude:: convert-multi-out.c
    :language: c

This program displays the following output:

.. literalinclude:: convert-multi-out.txt
    :language: text

Multi pass conversion on input
------------------------------

If you read your source data from a stream, you can do multiple passes
on the input.

For every pass, you need to call :c:func:`cahute_convert_text` with
the input set to your read buffer. On every pass, in nominal circumstances,
the function will return one of the following:

* :c:macro:`CAHUTE_ERROR_TERMINATED`, if a sentinel was found in the source
  data.
* :c:macro:`CAHUTE_ERROR_TRUNC`, if the input was found to be truncated,
  prompting you to do another pass while keeping the rest of the data.
* :c:macro:`CAHUTE_OK`, if all of the source data was converted, but no
  sentinel was found, prompting you to do another pass but not crash if
  no more bytes were available.

An example that reads from a memory area into a read buffer is the following:

.. literalinclude:: convert-multi-in.c
    :language: c

This program displays the following output:

.. literalinclude:: convert-multi-in.txt
    :language: text
