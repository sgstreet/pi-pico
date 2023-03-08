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

EXTRA_DEPS := ${SOURCE_DIR}/hello-world.mk
EXTRA_CLEAN := ${INSTALL_ROOT}/bin/hello-world

include ${PROJECT_ROOT}/tools/makefiles/project.mk

all: ${INSTALL_ROOT}/bin/hello-world
	@:

${INSTALL_ROOT}/bin/hello-world: ${CURDIR}/hello-world.elf
	@echo "INSTALL ${@}"
	$(INSTALL) -D ${CURDIR}/hello-world.elf ${@}

endif
