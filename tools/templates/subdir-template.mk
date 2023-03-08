#
# Copyright (C) 2017 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# subdir.mk
#
# Created on: Mar 16, 2017
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

where-am-i := $(lastword ${MAKEFILE_LIST})

SRC += $(wildcard $(dir $(where-am-i))*.c)
SRC += $(wildcard $(dir $(where-am-i))*.S)
SRC += $(wildcard $(dir $(where-am-i))*.s)