#
# Copyright (C) 2023 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# subdir.mk
#
# Created on: August 3, 2023
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

where-am-i := $(lastword ${MAKEFILE_LIST})

SRC += $(dir $(where-am-i))system-rp2040.c
SRC += $(dir $(where-am-i))irq-vector-smp.S
