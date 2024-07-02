#
# Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
#
# SPDX-License-Identifier: BSD-2-Clause
#

include_guard(DIRECTORY)

CAmkESAddTemplatesPath(.)

# Declare connectors with templates.
# We can rely on the naming scheme being consistent to allow for easier declaration.
foreach(
    connector
    IN
    ITEMS
    seL4DirectCall
    seL4GDB
    seL4GDBMem
    seL4Notification
    seL4NotificationBind
    seL4NotificationNative
    seL4NotificationQueue
    seL4RPCCall
    seL4DTBHardware
    seL4InitHardware
)
    DeclareCAmkESConnector(${connector} GENERATE)
endforeach()

#-------------------------------------------------------------------------------
DeclareCAmkESConnector(seL4SharedData GENERATE SYMMETRIC NO_HEADER)
DeclareCAmkESConnector(seL4DMASharedData GENERATE SYMMETRIC NO_HEADER)

#-------------------------------------------------------------------------------
# CamkeML
DeclareCAmkESConnector(
    seL4RPCCallNoType
    FROM
    seL4RPCCall-from.template.c
    TO
    seL4RPCCall-to.template.c
    CAKEML_TO
    seL4RPCCall-to.template.cakeml
)


DeclareCAmkESConnector(seL4RPCCall MODIFY_EXISTING CAKEML_TO seL4RPCCall-to.template.cakeml)


# Connectors with only FROM end interfaces
foreach(connector IN ITEMS seL4HardwareMMIO seL4HardwareIOPort)
    DeclareCAmkESConnector(${connector} GENERATE_FROM SYMMETRIC NO_HEADER )
endforeach()

DeclareCAmkESConnector(seL4DTBHW TYPE seL4DTBHardware GENERATE_TO SYMMETRIC NO_HEADER)


# Connectors with only TO end interfaces
foreach(connector IN ITEMS seL4HardwareInterrupt seL4IOAPICHardwareInterrupt)
    DeclareCAmkESConnector(${connector} GENERATE_FROM SYMMETRIC NO_HEADER)
endforeach()
