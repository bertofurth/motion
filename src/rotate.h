/*   This file is part of Motion.
 *
 *   Motion is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   Motion is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Motion.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 *  rotate.h
 *    Headers associated with functions in the rotate.c module.
 *    Copyright 2004-2005, Per Jonsson (per@pjd.nu)
 */

#ifndef _INCLUDE_ROTATE_H
#define _INCLUDE_ROTATE_H

/**
 * rotate_init
 *
 *  Sets up rotation data by allocating a temporary buffer for 90/270 degrees
 *  rotation, and by determining the right rotate-180-degrees function.
 *
 * Parameters:
 *
 *  cnt - current thread's context structure
 *
 * Returns: nothing
 */
void rotate_init(struct context *cnt);

/**
 * rotate_deinit
 *
 *  Frees memory allocated by rotate_init.
 *
 * Parameters:
 *
 *   cnt - current thread's context structure
 */
void rotate_deinit(struct context *cnt);

/**
 * rotate_img
 *
 *  Rotates the raw image stored in img according to the rotation data
 *  available in cnt. Rotation is performed clockwise. Supports 90,
 *  180 and 270 degrees rotation. 180 degrees rotation is performed
 *  in-place by simply reversing the image data, which is a very
 *  fast operation. 90 and 270 degrees rotation are performed using
 *  a temporary buffer and a somewhat more complicated algorithm,
 *  which makes them slower.
 *
 *  Note that to the caller, all rotations will seem as they are
 *  performed in-place.
 *
 * Parameters:
 *
 *   cnt - current thread's context structure
 *   img - pointer to the raw image data to rotate in place
 *   width - the *original* width of the image
 *   height - the *original* height of the image
 *
 * Returns:
 *
 *   0  - success. Image dimensions didn't change.
 *   1  - success. Image dimensions got swapped.
 *   -1 - failure (rare, shouldn't happen)
 */
int rotate_img(struct context *cnt, unsigned char *img, int width, int height);

/**
 * unrotate_pgm
 *
 *  Convert pgm data as output from get_pgm() from being
 *  based on the normal output picture dimensions to
 *  matching the captured image dimensions.
 *
 * Parameters:
 *
 *   cnt - the current thread's context structure
 *   pgm - pointer to pgm data as output by get_pgm()
 *   width - the *original* width of the pgm data
 *   height - the *original* height of the pgm data
 *
 * Returns:
 *
 *   0  - success. Image dimensions didn't change.
 *   1  - success. Image dimensions did change.
 *   -1 - failure (shouldn't happen)
 *
 */
int unrotate_pgm(struct context *cnt, unsigned char *pgm, int width, int height);

#endif
