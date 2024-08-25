/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  Copyright: @2024 Stephen Street (stephen@redrocketcomputing.com)
 */

#ifndef _REF_H_
#define _REF_H_

#include <assert.h>
#include <stdbool.h>
#include <stdatomic.h>

struct ref
{
	atomic_int count;
	void (*release)(struct ref *ref);
};

static inline void ref_ini(struct ref *ref, void (*release)(struct ref *ref), int initial)
{
	assert(ref != 0);

	ref->count = initial;
	ref->release = release;
}

static inline void ref_up(struct ref *ref)
{
	assert(ref != 0);

	atomic_fetch_add(&ref->count, 1);
}

static inline bool ref_down(struct ref *ref)
{
	assert(ref != 0);

	/* Update the reference count */
	if (atomic_fetch_sub(&ref->count, 1) < 2) {

		/* Release? */
		if (ref->release)
			ref->release(ref);

		/* All done with object */
		return true;
	}

	/* Still alive */
	return false;
}

#endif
