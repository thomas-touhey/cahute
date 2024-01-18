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

#ifndef CAHUTE_VERSION_H
#define CAHUTE_VERSION_H 1
#include "cdefs.h"

CAHUTE_BEGIN_NAMESPACE
CAHUTE_DECLARE_TYPE(cahute_os_version)

/* CASIO's OS versions look like 'MM.mm.ABCD', where 'MM' is the major, 'mm'
 * is the minor, and 'FFFF' contain various information.
 *
 * 'A' is the zone (localization) of the OS. It is one the following: */
#define CAHUTE_VERSION_ZONE_NONE 0x00 /* (international) */
#define CAHUTE_VERSION_ZONE_AUS  0x01 /* Australia */
#define CAHUTE_VERSION_ZONE_FR   0x02 /* France */
#define CAHUTE_VERSION_ZONE_NAM  0x03 /* North America */
#define CAHUTE_VERSION_ZONE_CH   0x04 /* China */
#define CAHUTE_VERSION_ZONE_SING 0x05 /* Singapour */

/* 'B' is the math input/output features of the OS (OS type).
 * It is one of the following: */
#define CAHUTE_VERSION_MATH_SLIM    0x01 /* Slim */
#define CAHUTE_VERSION_MATH_ALL     0x02 /* All features (fx-9860GII-2) */
#define CAHUTE_VERSION_MATH_REDUCED 0x03 /* Reduced features (fx-7400GII) */
#define CAHUTE_VERSION_MATH_NONE    0x07 /* No math features? */

/* 'C' is the in-development status of the OS.
 * It is one of the following: */
#define CAHUTE_VERSION_STATUS_STANDARD 0x00 /* Standard build */
#define CAHUTE_VERSION_STATUS_INDEV    0x01 /* In-development/special build */

/* 'D' is the hardware platform ID for which the OS is built.
 * It is one of the following: */
#define CAHUTE_VERSION_PLATFORM_BASIC   0x00 /* SH7337/SH7355 */
#define CAHUTE_VERSION_PLATFORM_SPECIAL 0x01 /* SH7305 */
#define CAHUTE_VERSION_PLATFORM_PRIZM   0x02 /* SH7305 (Prizm) */

struct cahute_os_version {
    int cahute_version_major;
    int cahute_version_minor;
    int cahute_version_zone;
    int cahute_version_math;
    int cahute_version_status;
    int cahute_version_platform;
};

CAHUTE_END_NAMESPACE

#endif /* CAHUTE_VERSION_H */
