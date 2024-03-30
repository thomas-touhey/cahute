/* ****************************************************************************
 * Copyright (C) 2024 Thomas Touhey <thomas@touhey.fr>
 *
 * This software is governed by the CeCILL 2.1 license under French law and
 * abiding by the rules of distribution of free software. You can use, modify
 * and/or redistribute the software under the terms of the CeCILL 2.1 license
 * as circulated by CEA, CNRS and INRIA at the following
 * URL: https://cecill.info
 *
 * As a counterpart to the access to the source code and rights to copy, modify
 * and redistribute granted by the license, users are provided only with a
 * limited warranty and the software's author, the holder of the economic
 * rights, and the successive licensors have only limited liability.
 *
 * In this respect, the user's attention is drawn to the risks associated with
 * loading, using, modifying and/or developing or reproducing the software by
 * the user in light of its specific status of free software, that may mean
 * that it is complicated to manipulate, and that also therefore means that it
 * is reserved for developers and experienced professionals having in-depth
 * computer knowledge. Users are therefore encouraged to load and test the
 * software's suitability as regards their requirements in conditions enabling
 * the security of their systems and/or data to be ensured and, more generally,
 * to use and operate it in the same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL 2.1 license and that you accept its terms.
 * ************************************************************************* */

#ifndef CAHUTE_PICTURE_H
#define CAHUTE_PICTURE_H 1
#include "cdefs.h"

CAHUTE_BEGIN_NAMESPACE
CAHUTE_DECLARE_TYPE(cahute_frame)

#define CAHUTE_PICTURE_FORMAT_1BIT_MONO         1
#define CAHUTE_PICTURE_FORMAT_1BIT_MONO_CAS50   2
#define CAHUTE_PICTURE_FORMAT_1BIT_DUAL         3
#define CAHUTE_PICTURE_FORMAT_1BIT_TRIPLE_CAS50 4
#define CAHUTE_PICTURE_FORMAT_4BIT_RGB_PACKED   5
#define CAHUTE_PICTURE_FORMAT_16BIT_R5G6B5      6
#define CAHUTE_PICTURE_FORMAT_32BIT_ARGB_HOST   7

struct cahute_frame {
    int cahute_frame_width;
    int cahute_frame_height;
    int cahute_frame_format;
    cahute_u8 const *cahute_frame_data;
};

CAHUTE_BEGIN_DECLS

CAHUTE_EXTERN(int)
cahute_convert_picture(
    void *cahute__dest,
    int cahute__dest_format,
    void const *cahute__src,
    int cahute__src_format,
    int cahute__width,
    int cahute__height
);

CAHUTE_EXTERN(int)
cahute_convert_picture_from_frame(
    void *cahute__dest,
    int cahute__dest_format,
    cahute_frame const *cahute__frame
);

CAHUTE_END_DECLS
CAHUTE_END_NAMESPACE

#endif /* CAHUTE_PICTURE_H */
