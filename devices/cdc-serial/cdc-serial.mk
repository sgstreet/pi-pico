#
# Copyright (C) 2017 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# template-project.mk
#
# Created on: Mar 16, 2017
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

EXTRA_DEPS += ${SOURCE_DIR}/cdc-serial.mk

include ${PROJECT_ROOT}/tools/makefiles/project.mk

CPPFLAGS += -DPICO_NO_HARDWARE -DCFG_TUSB_CONFIG_FILE="<${SOURCE_DIR}/../usb-device/tusb_config.h>"
CPPFLAGS += -I${PROJECT_ROOT}/runtime/include -I${SOURCE_DIR}/../usb-device/tinyusb

endif