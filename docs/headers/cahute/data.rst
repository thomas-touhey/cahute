``<cahute/data.h>`` -- Calculator data resource and methods for Cahute
======================================================================

This header declares link-related utilities for Cahute.

Type definitions
----------------

.. c:struct:: cahute_data

    Main memory data.

    .. c:member:: cahute_data *cahute_data_next

        Next node in the collection.

        .. todo:: Describe the role of this property.

    .. c:member:: int cahute_data_type

        Type of the data.

        .. c:macro:: CAHUTE_DATA_TYPE_PROGRAM

            Constant representing the :ref:`data-program` data type.

    .. c:member:: union cahute__data_content cahute_data_content

        Conditional data contents, depending on the data type.

        .. c:member:: struct cahute__data_content_program \
            cahute_data_content_program

            Data contents, if the type is :c:macro:`CAHUTE_DATA_TYPE_PROGRAM`.

            .. c:member:: int cahute_data_content_program_encoding

                Text encoding used for the program name, password and content,
                as a ``CAHUTE_TEXT_ENCODING_*`` constant.

            .. c:member:: size_t cahute_data_content_program_name_size

                Size of the program name, in bytes.

                If this is set to 0,
                :c:member:`cahute_data_content_program_name` may be ``NULL``
                or undefined.

            .. c:member:: void *cahute_data_content_program_name

                Program name, in the encoding set in
                :c:member:`cahute_data_content_program_encoding`.

            .. c:member:: size_t cahute_data_content_program_password_size

                Size of the program password, in bytes.

                If this is set to 0,
                :c:member:`cahute_data_content_program_password` may be
                ``NULL`` or undefined.

            .. c:member:: void *cahute_data_content_program_password

                Program password, in the encoding set in
                :c:member:`cahute_data_content_program_encoding`.

            .. c:member:: size_t cahute_data_content_program_size

                Size of the program contents, in bytes.

                If this is set to 0,
                :c:member:`cahute_data_content_program_content` may be
                ``NULL`` or undefined.

            .. c:member:: void *cahute_data_content_program_content

                Program content, in the encoding set in
                :c:member:`cahute_data_content_program_encoding`.

Function declarations
---------------------

.. c:function:: void cahute_destroy_data(cahute_data *data)

    Deallocate data.

    :param data: Data to deallocate.
    :return: The error, or 0 if the operation was successful.
