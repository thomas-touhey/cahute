.. _header-cahute-text:

``<cahute/text.h>`` -- Text encoding related utilities for Cahute
=================================================================

Macro definitions
-----------------

``CAHUTE_TEXT_ENCODING_*`` are constants representing how a given
picture's data is encoded.

.. c:macro:: CAHUTE_TEXT_ENCODING_LEGACY_8

    Constant representing the :ref:`text-encoding-fc8` with
    the legacy character table.

.. c:macro:: CAHUTE_TEXT_ENCODING_LEGACY_16_HOST

    Constant representing the :ref:`text-encoding-fc16` with
    the legacy character table, and host endianness.

.. c:macro:: CAHUTE_TEXT_ENCODING_LEGACY_16_BE

    Constant representing the :ref:`text-encoding-fc16` with
    the legacy character table, and big endian.

.. c:macro:: CAHUTE_TEXT_ENCODING_LEGACY_16_LE

    Constant representing the :ref:`text-encoding-fc16` with
    the legacy character table, and little endian.

.. c:macro:: CAHUTE_TEXT_ENCODING_9860_8

    Constant representing the :ref:`text-encoding-fc8` with
    the fx-9860G character table.

.. c:macro:: CAHUTE_TEXT_ENCODING_9860_16_HOST

    Constant representing the :ref:`text-encoding-fc16` with
    the fx-9860G character table, and host endianness.

.. c:macro:: CAHUTE_TEXT_ENCODING_9860_16_BE

    Constant representing the :ref:`text-encoding-fc16` with
    the fx-9860G character table, and big endian.

.. c:macro:: CAHUTE_TEXT_ENCODING_9860_16_LE

    Constant representing the :ref:`text-encoding-fc16` with
    the fx-9860G character table, and little endian.

.. c:macro:: CAHUTE_TEXT_ENCODING_CAT

    Constant representing the :ref:`text-encoding-cat`.

.. c:macro:: CAHUTE_TEXT_ENCODING_CTF

    Constant representing the :ref:`text-encoding-ctf`.

.. c:macro:: CAHUTE_TEXT_ENCODING_UTF32_HOST

    Constant representing the :ref:`text-encoding-utf32`, with
    host endianness.

.. c:macro:: CAHUTE_TEXT_ENCODING_UTF32_BE

    Constant representing the :ref:`text-encoding-utf32`, with
    big endian.

.. c:macro:: CAHUTE_TEXT_ENCODING_UTF32_LE

    Constant representing the :ref:`text-encoding-utf32`, with
    little endian.

.. c:macro:: CAHUTE_TEXT_ENCODING_UTF8

    Constant representing the :ref:`text-encoding-utf8`.

Function declarations
---------------------

.. c:function:: int cahute_convert_text(void **bufp, size_t *buf_sizep, \
    void const **datap, size_t *data_sizep, int dest_encoding, \
    int source_encoding)

    Convert text from one encoding to another.

    .. note::

        When :c:macro:`CAHUTE_TEXT_ENCODING_UTF32_HOST`,
        :c:macro:`CAHUTE_TEXT_ENCODING_UTF32_BE`,
        :c:macro:`CAHUTE_TEXT_ENCODING_UTF32_LE` or
        :c:macro:`CAHUTE_TEXT_ENCODING_UTF8` is used as the destination
        encoding, **Normalization Form C (NFC)** is employed; see
        `Unicode Normalization Forms`_ for more information.

    Errors you can expect from this function are the following:

    :c:macro:`CAHUTE_OK`
        The conversion has finished successfully, and there is no
        more bytes in the input buffer to read.

    :c:macro:`CAHUTE_ERROR_TERMINATED`
        A sentinel has been found, and the conversion has been interrupted.

        .. note::

            If this error is raised, ``*datap`` is set to **after** the
            sentinel, and ``*data_sizep`` is set accordingly.

            This is useful in case you have multiple text blobs placed
            back-to-back.

    :c:macro:`CAHUTE_ERROR_SIZE`
        The destination buffer had insufficient space, and the procedure
        was interrupted.

    :c:macro:`CAHUTE_ERROR_TRUNC`
        The source data had an incomplete sequence, and the procedure
        was interrupted.

    :c:macro:`CAHUTE_ERROR_INVALID`
        The source data contained an unknown or invalid sequence, and
        the procedure was interrupted.

    :c:macro:`CAHUTE_ERROR_INCOMPAT`
        The source data contained a sequence that could not be translated
        to the destination encoding.

    At the end of its process, this function updates ``*bufp``, ``*buf_sizep``,
    ``*datap`` and ``*data_sizep`` to the final state of the function,
    even in case of error, so that:

    * You can determine how much of the destination buffer was filled,
      by substracting the final buffer size to the original buffer size.
    * In case of :c:macro:`CAHUTE_ERROR_SIZE`, you can get the place at
      which to get the leftover bytes in the source data.
    * In case of :c:macro:`CAHUTE_ERROR_TRUNC`, you can get the place at
      which to get the leftover bytes in the source data to complete with
      additional data for the next conversion.
    * In case of :c:macro:`CAHUTE_ERROR_INVALID` or
      :c:macro:`CAHUTE_ERROR_INCOMPAT`, you can get the place of the
      problematic input sequence.

    Currently supported conversions are the following:

    .. list-table::
        :header-rows: 1
        :width: 100%

        * - | Src. ⯈
            | ▼ Dst.
          - ``LEGACY_*``
          - ``9860_*``
          - ``CAT``
          - ``CTF``
          - ``UTF*``
        * - ``LEGACY_*``
          - x
          - x
          -
          -
          -
        * - ``9860_*``
          - x
          - x
          -
          -
          -
        * - ``CAT``
          -
          -
          -
          -
          -
        * - ``CTF``
          -
          -
          -
          -
          -
        * - ``UTF*``
          - x
          - x
          -
          -
          - x

    For specific guides on how to use this function, see
    :ref:`guide-developer-convert-text`.

    :param bufp: Pointer to the destination buffer pointer.
    :param buf_sizep: Pointer to the destination buffer size.
    :param datap: Pointer to the source data pointer.
    :param data_sizep: Pointer to the source data size.
    :param dest_encoding: Destination encoding.
    :param source_encoding: Source encoding.
    :return: Error, or 0 if the operation was successful.

.. c:function:: int cahute_convert_to_utf8(char *buf, size_t buf_size, \
    void const *data, size_t data_size, int encoding)

    Convert the provided data to UTF-8, and place a terminating NUL character.

    This is a utility that calls :c:func:`cahute_convert_text`, for
    simple scripts using the Cahute library.

    :param buf: Destination buffer.
    :param buf_size: Destination buffer size.
    :param data: Source data.
    :param data_size: Size of the source data.
    :param encoding: Encoding of the source data.
    :return: Error, or 0 if the operation was successful.

.. _Unicode Normalization Forms: https://www.unicode.org/reports/tr15/
