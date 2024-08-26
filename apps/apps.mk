#
# Copyright (C) 2017 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# apps.mk
#
# Created on: April 26, 2023
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)

debug:
motor-controller: debug
target: motor-controller

include ${PROJECT_ROOT}/tools/makefiles/target.mk
else
include ${PROJECT_ROOT}/tools/makefiles/project.mk
endif
