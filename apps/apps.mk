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

half-duplex-rx-test: devices
half-duplex-test: devices
leg-setup: devices svc
target: half-duplex-test half-duplex-rx-test leg-setup rocket-hex

include ${PROJECT_ROOT}/tools/makefiles/target.mk

else

include ${PROJECT_ROOT}/tools/makefiles/project.mk

endif
