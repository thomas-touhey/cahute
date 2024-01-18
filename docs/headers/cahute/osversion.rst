``<cahute/osversion.h>`` -- CASIO OS version format utilities for Cahute
========================================================================

Macro definitions
-----------------

``CAHUTE_VERSION_ZONE_*`` represent geographical zones
(markets) for which the software is produced.

.. c:macro:: CAHUTE_VERSION_ZONE_NONE

    International.

.. c:macro:: CAHUTE_VERSION_ZONE_AUS

    Australia.

.. c:macro:: CAHUTE_VERSION_ZONE_FR

    France.

.. c:macro:: CAHUTE_VERSION_ZONE_NAM

    North America.

.. c:macro:: CAHUTE_VERSION_ZONE_CH

    China.

.. c:macro:: CAHUTE_VERSION_ZONE_SING

    Singapour.

``CAHUTE_VERSION_MATH_*`` represent the available math
input/output features for the software.

.. c:macro:: CAHUTE_VERSION_MATH_SLIM

    Slim.

.. c:macro:: CAHUTE_VERSION_MATH_ALL

    All features (fx-9860GII-2).

.. c:macro:: CAHUTE_VERSION_MATH_REDUCED

    Reduced features (fx-7400GII)

.. c:macro:: CAHUTE_VERSION_MATH_NONE

    No features.

``CAHUTE_VERSION_STATUS_*`` represent the build type for
the software.

.. c:macro:: CAHUTE_VERSION_STATUS_STANDARD

    Standard build.

.. c:macro:: CAHUTE_VERSION_STATUS_INDEV

    Special / Development build.

``CAHUTE_VERSION_PLATFORM_*`` represent the hardware platform
on which the software runs.

.. c:macro:: CAHUTE_VERSION_PLATFORM_BASIC

    SH7337 / SH7355.

.. c:macro:: CAHUTE_VERSION_PLATFORM_SPECIAL

    SH7305.

.. c:macro:: CAHUTE_VERSION_PLATFORM_PRIZM

    SH7305 (Prizm).

Type definitions
----------------

.. c:struct:: cahute_os_version

    CASIO OS version structure, determined from a version string.

    .. c:member:: int cahute_version_major

        Major version.

    .. c:member:: int cahute_version_minor

        Minor version.

    .. c:member:: cahute_version_zone cahute_version_zone

        Geographical zone (market).

    .. c:member:: cahute_version_math cahute_version_math

        Available math I/O features.

    .. c:member:: cahute_version_status cahute_version_status

        Build type.

    .. c:member:: cahute_version_platform cahute_version_platform

        Hardware platform.
