/*
 * Copyright (C) 2017 Red Rocket Computing, LLC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * diag-board.c
 *
 * Created on: Mar 8, 2017
 *     Author: Stephen Street (stephen@redrocketcomputing.com)
 */

#include <diag/diag.h>
#include <board/board.h>

int diag_getc(void)
{
	return board_getchar(0);
}

int diag_putc(int c)
{
	return board_putchar(c);
}

int diag_puts(const char *s)
{
	return board_puts(s);
}


