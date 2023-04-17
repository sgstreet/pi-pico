#
# Copyright (C) 2017 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# commom.mk
#
# Created on: Mar 16, 2017
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

-include ${PROJECT_ROOT}/tools/makefiles/${ARCH_CROSS}.mk
-include ${PROJECT_ROOT}/tools/makefiles/${CHIP_TYPE}.mk
-include ${PROJECT_ROOT}/tools/makefiles/${BOARD_TYPE}.mk
-include ${PROJECT_ROOT}/tools/makefiles/${BUILD_TYPE}.mk

