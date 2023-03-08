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

EXTRA_OBJ_DEPS := ${CURDIR}/half-duplex.pio.h
EXTRA_CLEAN := ${CURDIR}/half-duplex.pio.h

include ${PROJECT_ROOT}/tools/makefiles/project.mk

CPPFLAGS += -DPICO_NO_HARDWARE -I${CURDIR}

endif
