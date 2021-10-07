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
 *    rotate.c
 *
 *    Module for handling image rotation.
 *
 *    Copyright 2004-2005, Per Jonsson (per@pjd.nu)
 *
 *    Image rotation is a feature of Motion that can be used when the
 *    camera is mounted upside-down or on the side. The module only
 *    supports rotation in multiples of 90 degrees. Using rotation
 *    increases the Motion CPU usage slightly.
 *
 *    Version history:
 *      v6 (29-Aug-2005) - simplified the code as Motion now requires
 *                         that width and height are multiples of 16
 *      v5 (3-Aug-2005)  - cleanup in code comments
 *                       - better adherence to coding standard
 *                       - fix for __bswap_32 macro collision
 *                       - fixed bug where initialization would be
 *                         incomplete for invalid degrees of rotation
 *                       - now uses MOTION_LOG for error reporting
 *      v4 (26-Oct-2004) - new fix for width/height from imgs/conf due to
 *                         earlier misinterpretation
 *      v3 (11-Oct-2004) - cleanup of width/height from imgs/conf
 *      v2 (26-Sep-2004) - separation of capture/internal dimensions
 *                       - speed optimization, including bswap
 *      v1 (28-Aug-2004) - initial version
 */
#include "translate.h"
#include "motion.h"
#include "util.h"
#include "logger.h"
#include "rotate.h"
#include <stdint.h>
#if defined(__APPLE__)
    #include <libkern/OSByteOrder.h>
    #define bswap_32(x) OSSwapInt32(x)
#elif defined(__FreeBSD__)
    #include <sys/endian.h>
    #define bswap_32(x) bswap32(x)
#elif defined(__OpenBSD__)
    #include <sys/types.h>
    #define bswap_32(x) swap32(x)
#elif defined(__NetBSD__)
    #include <sys/bswap.h>
    #define bswap_32(x) bswap32(x)
#else
    #include <byteswap.h>
#endif

/**
 * reverse_inplace_quad
 *
 *  Reverses a block of memory in-place, 4 bytes at a time. This function
 *  requires the uint32_t type, which is 32 bits wide.
 *
 * Parameters:
 *
 *   src  - the memory block to reverse
 *   size - the size (in bytes) of the memory block
 *
 * Returns: nothing
 */
static void reverse_inplace_quad(unsigned char *src, int size)
{
    uint32_t *nsrc = (uint32_t *)src;              /* first quad */
    uint32_t *ndst = (uint32_t *)(src + size - 4); /* last quad */
    register uint32_t tmp;

    while (nsrc < ndst) {
        tmp = bswap_32(*ndst);
        *ndst-- = bswap_32(*nsrc);
        *nsrc++ = tmp;
    }
}

static void flip_inplace_horizontal(unsigned char *src, int width, int height)
{
    uint8_t *nsrc, *ndst;
    register uint8_t tmp;
    int l,w;

    for(l=0; l < height/2; l++) {
        nsrc = (uint8_t *)(src + l*width);
        ndst = (uint8_t *)(src + (width*(height-l-1)));
        for(w=0; w < width; w++) {
            tmp =*ndst;
            *ndst++ = *nsrc;
            *nsrc++ = tmp;
        }
    }

}

static void flip_inplace_vertical(unsigned char *src, int width, int height)
{
    uint8_t *nsrc, *ndst;
    register uint8_t tmp;
    int l;

    for(l=0; l < height; l++) {
        nsrc = (uint8_t *)src + l*width;
        ndst = nsrc + width - 1;
        while (nsrc < ndst) {
            tmp = *ndst;
            *ndst-- = *nsrc;
            *nsrc++ = tmp;
        }
    }
}

/**
 * rot90cw
 *
 *  Performs a 90 degrees clockwise rotation of the memory block pointed to
 *  by src. The rotation is NOT performed in-place; dst must point to a
 *  receiving memory block the same size as src.
 *
 * Parameters:
 *
 *   src    - pointer to the memory block (image) to rotate clockwise
 *   dst    - where to put the rotated memory block
 *   size   - the size (in bytes) of the memory blocks (both src and dst)
 *   width  - the width of the memory block when seen as an image
 *   height - the height of the memory block when seen as an image
 *
 * Returns: nothing
 */
static void rot90cw(unsigned char *src, register unsigned char *dst
            ,int size, int width, int height)
{
    unsigned char *endp;
    register unsigned char *base;
    int j;

    endp = src + size;
    for (base = endp - width; base < endp; base++) {
        src = base;
        for (j = 0; j < height; j++, src -= width) {
            *dst++ = *src;
        }
    }
}

/**
 * rot90ccw
 *
 *  Performs a 90 degrees counterclockwise rotation of the memory block pointed
 *  to by src. The rotation is not performed in-place; dst must point to a
 *  receiving memory block the same size as src.
 *
 * Parameters:
 *
 *   src    - pointer to the memory block (image) to rotate counterclockwise
 *   dst    - where to put the rotated memory block
 *   size   - the size (in bytes) of the memory blocks (both src and dst)
 *   width  - the width of the memory block when seen as an image
 *   height - the height of the memory block when seen as an image
 *
 * Returns: nothing
 */
static inline void rot90ccw(unsigned char *src, register unsigned char *dst
            ,int size, int width, int height)
{
    unsigned char *endp;
    register unsigned char *base;
    int j;

    endp = src + size;
    dst = dst + size - 1;
    for (base = endp - width; base < endp; base++) {
        src = base;
        for (j = 0; j < height; j++, src -= width) {
            *dst-- = *src;
        }
    }
}

/**
 * rotate_init
 *
 *  Initializes rotation data - allocates memory and determines which function
 *  to use for 180 degrees rotation.
 *
 * Parameters:
 *
 *   cnt - the current thread's context structure
 *
 * Returns: nothing
 */
void rotate_init(struct context *cnt)
{
    int size_norm, size_high, needed_buffer_size;

    /*
     * Assign the value in conf.rotate to rotate_data.degrees. This way,
     * we have a value that is safe from changes caused by motion-control.
     */
    if ((cnt->conf.rotate % 90) > 0) {
        MOTION_LOG(WRN, TYPE_ALL, NO_ERRNO
            ,_("Config option \"rotate\" not a multiple of 90: %d")
            ,cnt->conf.rotate);
        cnt->conf.rotate = 0;     /* Disable rotation. */
        cnt->rotate_data.degrees = 0; /* Force return below. */
    } else {
        cnt->rotate_data.degrees = cnt->conf.rotate % 360; /* Range: 0..359 */
    }

    if (cnt->conf.flip_axis[0]=='h') {
        cnt->rotate_data.axis = FLIP_TYPE_HORIZONTAL;
    } else if (cnt->conf.flip_axis[0]=='v') {
        cnt->rotate_data.axis = FLIP_TYPE_VERTICAL;
    } else {
        cnt->rotate_data.axis = FLIP_TYPE_NONE;
    }

    /*
     * Upon entrance to this function, imgs.width and imgs.height contain the
     * capture dimensions (as set in the configuration file, or read from a
     * netcam source).
     *
     * If rotating 90 or 270 degrees, the capture dimensions and output dimensions
     * are not the same. Output dimensions are set in imgs.display_width and
     * imgs.display_height. Same for *_high for hires image dimensions.
     */

    size_norm = cnt->imgs.width * cnt->imgs.height * 3 / 2;
    size_high = cnt->imgs.width_high * cnt->imgs.height_high * 3 / 2;

    if (cnt->rotate_data.degrees == 0 || cnt->rotate_data.degrees == 180) {
	cnt->imgs.display_width = cnt->imgs.width;
	cnt->imgs.display_height = cnt->imgs.height;
	if (size_high > 0 ) {
	    cnt->imgs.display_width_high = cnt->imgs.width_high;
	    cnt->imgs.display_height_high = cnt->imgs.height_high;
	}
    } else {
	/*
	 * Allocate memory if rotating 90 or 270 degrees, because those rotations
	 * cannot be performed in-place (they can, but it would be too slow).
	 */
	needed_buffer_size = size_high > size_norm ? size_high : size_norm;
	if (needed_buffer_size > cnt->imgs.common_buffer_size) {
	    free(cnt->imgs.common_buffer);
	    cnt->imgs.common_buffer_size = needed_buffer_size;
	    cnt->imgs.common_buffer = mymalloc(needed_buffer_size);
	}
	cnt->imgs.display_width = cnt->imgs.height;
	cnt->imgs.display_height = cnt->imgs.width;
	if (size_high > 0 ) {
	    cnt->imgs.display_width_high = cnt->imgs.height_high;
	    cnt->imgs.display_height_high = cnt->imgs.width_high;
	}
    }
}

/**
 * rotate_deinit
 *
 *  Frees resources previously allocated by rotate_init.
 *
 * Parameters:
 *
 *   cnt - the current thread's context structure
 *
 * Returns: nothing
 */
void rotate_deinit(struct context *cnt)
{

}


/**
 * rotate_img
 *
 *  Main entry point for rotation.
 *
 * Parameters:
 *
 *   cnt - the current thread's context structure
 *   img - pointer to the raw image data to rotate in place
 *   width - the *original* width of the image
 *   height - the *original* height of the image
 *
 * Returns:
 *
 *   0  - success. Image dimensions didn't change.
 *   1  - success. Image dimensions did change.
 *   -1 - failure (shouldn't happen)
 *
 * Future : Possibly make a copy of an image after
 * rotation to cater for the case where two different 
 * features want the same image but rotated differently.
 * 
 */

int rotate_img(struct context *cnt, unsigned char *img, int width, int height){
    /*
     * The image format is YUV 4:2:0 planar, which has the pixel
     * data is divided in three parts:
     *    Y - width x height bytes
     *    U - width x height / 4 bytes
     *    V - as U
     */

    int wh, wh4 = 0, w2 = 0, h2 = 0;  /* width * height, width * height / 4 etc. */
    int size, deg;
    enum FLIP_TYPE axis;
    unsigned char *temp_buff;

    if (cnt->rotate_data.degrees == 0 && cnt->rotate_data.axis == FLIP_TYPE_NONE) return 0;

    deg = cnt->rotate_data.degrees;
    axis = cnt->rotate_data.axis;
    wh4 = 0;
    w2 = 0;
    h2 = 0;
    temp_buff = cnt->imgs.common_buffer;

    memset (temp_buff, 0xff, cnt->imgs.common_buffer_size); /* BERTO DELME */
    MOTION_LOG(DBG, TYPE_ALL, NO_ERRNO
	       ,_("Rotating image height %d width %d degrees %d axis %d")
	       ,height, width, deg, axis);
    /*
     * Pre-calculate some stuff:
     *  wh   - size of the Y plane
     *  size - size of the entire memory block
     *  wh4  - size of the U plane, and the V plane
     *  w2   - width of the U plane, and the V plane
     *  h2   - as w2, but height instead
     */
    wh = width * height;
    size = wh * 3 / 2;
    wh4 = wh / 4;
    w2 = width / 2;
    h2 = height / 2;

    switch (axis) {
    case FLIP_TYPE_HORIZONTAL:
	flip_inplace_horizontal(img,width, height);
	flip_inplace_horizontal(img + wh, w2, h2);
	flip_inplace_horizontal(img + wh + wh4, w2, h2);
	break;
    case FLIP_TYPE_VERTICAL:
	flip_inplace_vertical(img,width, height);
	flip_inplace_vertical(img + wh, w2, h2);
	flip_inplace_vertical(img + wh + wh4, w2, h2);
	break;
    default:
	break;
    }

    switch (deg) {
    case 0:
	break;
    case 90:
	rot90cw(img, temp_buff, wh, width, height);
	rot90cw(img + wh, temp_buff + wh, wh4, w2, h2);
	rot90cw(img + wh + wh4, temp_buff + wh + wh4, wh4, w2, h2);
	memcpy(img, temp_buff, size);
	return 1;  /* Dimensions changed */
	break;
    case 180:
	reverse_inplace_quad(img, wh);
	reverse_inplace_quad(img + wh, wh4);
	reverse_inplace_quad(img + wh + wh4, wh4);
	break;
    case 270:
	rot90ccw(img, temp_buff, wh, width, height);
	rot90ccw(img + wh, temp_buff + wh, wh4, w2, h2);
	rot90ccw(img + wh + wh4, temp_buff + wh + wh4, wh4, w2, h2);
	memcpy(img, temp_buff, size);
	return 1;  /* Dimensions changed */
	break;
    default:
	/* Invalid */
	return -1;
    }
    return 0;
} /* rotate_img() */



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

int unrotate_pgm(struct context *cnt, unsigned char *pgm, int width, int height){

    int wh, deg;
    enum FLIP_TYPE axis;
    unsigned char *temp_buff;

    if (cnt->rotate_data.degrees == 0 && cnt->rotate_data.axis == FLIP_TYPE_NONE) return 0;

    deg = cnt->rotate_data.degrees;
    axis = cnt->rotate_data.axis;
    temp_buff = cnt->imgs.common_buffer;

    MOTION_LOG(DBG, TYPE_ALL, NO_ERRNO
	       ,_("Unrotating pgm height %d width %d degrees %d axis %d")
	       ,height, width, deg, axis);
    /*
     * Pre-calculate some stuff:
     *  wh   - size of the pgm data
     */
    wh = width * height;

    switch (axis) {
    case FLIP_TYPE_HORIZONTAL:
	flip_inplace_horizontal(pgm, width, height);
	break;
    case FLIP_TYPE_VERTICAL:
	flip_inplace_vertical(pgm, width, height);
	break;
    default:
	break;
    }

    /*
     * Remember we are rotating "backwards" here
     * so we are rotating counter clockwise.
     */
    switch (deg) {
    case 0:
	break;
    case 90:
	rot90ccw(pgm, temp_buff, wh, width, height);
	memcpy(pgm, temp_buff, wh);
	return 1;  /* Dimensions changed */
	break;
    case 180:
	reverse_inplace_quad(pgm, wh);
	break;
    case 270:
	rot90cw(pgm, temp_buff, wh, width, height);
	memcpy(pgm, temp_buff, wh);
	return 1;  /* Dimensions changed */
	break;
    default:
	/* Invalid */
	return -1;
    }
    return 0;
} /* unrotate_pgm */

