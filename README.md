Motion with "Just in Time" Rotation
===================================

The original motion README.md file is below
these notes.

# Rotate image/Video only when saving or streaming

This fork of motion is a proposed method of implementing
the changes suggested in 

https://github.com/Motion-Project/motion/issues/1321

The suggestion was to only perform the CPU intensive
task of image rotation when images were actually
being viewed via a web stream or when they are being
saved.

This fork proposes a change to motion where it will store
image files internally in the original / captured /
virgin format and only perform rotation at the point where
images are written to files or streamed.

A brief test of these changes using "rotation 90" reduced
motion's CPU utilization in the "idle state" by about 20% on
what it otherwise may have been. "Idle state" meams that
no web streams are being viewed and no event triggered output 
files are being generated.

There are a number of other benefits which I'll outline
below but first it's important to note some of the drawbacks 
of these proposed changes.

## Issues
### mask_file and mask_privacy
The basis of the new proposal is that images be stored internally
within motion in their unrotated, unprocessed form.

For this reason, masks need to be applied to images using the
source dimensions, not the displayed or post-rotation dimensions.

Since "mask_file" and "mask_privacy" are documented as needing to
be the same dimensions as the output file we have to internally
"unrotate" the mask files. TODO TODO TODO.

Personally, I think it could potentially be better going forward to
specify that masks apply to the source / captured images because
this way a user can arbitrarily change the rotation or axis of the
displayed image without having to worry about making corresponding 
changes to the privacy mask.

TODO : Maybe have a "privacy_mask_captured" and "mask_file_captured"
boolean (on / off) configuration option to specify that mask files
apply to the captured image dimentions.

### "Debug" streams and images not rotated
The "source" (virgin) and "motion" streams are *not* rotated since
these types of images are now stored internally as unrotated.

It would not be difficult to rotate them however, in my view, this
would be contrary to the purpose of these streams which, as I understand
it, are used for debugging. If they were rotated then they'd be less
"pure" and any problems with the rotation system would impact them.

This also goes for "picture_output_motion" and "movie_output_motion".

### Error messages and debug text overlays
Embedded error messages such as "CONNECTION TO CAMERA LOST" or text
debugs shown when using log level "DBG" are applied to images *before*
they are rotated. This means that if an image is subsequently rotated 
after one of these types of text are overlaid then that text will also
get rotated. Fixing this might be quite tedious so hopefully it's not a
deal breaker. 

### %w and %h conversion specifiers
The %w and %h conversion specifiers will show the width and height 
dimensions of a normal saved image. If these are used in the filename of a 
"motion" image or movie when the normal image has been rotated 90 or 270
degrees then they may be incorrect.

Hopefully this is a minor issue that does not seriously impact anyone.

### "netcam_hires" streams
Originally motion did not apply overlay text to "netcam_hires" images.

These changes mean that text overlays *will* be applied to "netcam_hires"
stream outputs but, as before, not for the video file when 
"movie_passthrough" is specified.

Since the "text_scale" factor will be the same as for the normal
resolution images the text might appear to be very small on the
hires images.

Potential alternatives include
* Disable text on hires images (like before)
* New config option "text_scale_hires" to specify a hires specific scale factor with 0 meaning "no text".
* Automatically calculate hires scale factor based on the ratio of hires to normal image dimensions.


## The changes
### Images are stored internally as unrotated and unannotated

### Rotation buffers are consolidated
Previously the rotation functions had their own buffers to perform
90 and 270 degree rotation. These have been removed and the common_buffer
is now used for rotation. Saves about BERTO BERTO BERTO measure!!

## Future work
### Different text/rotation for streams and saved images
One potential extra feature that could be easily introduced with this
change is users could optionally apply separate rotation and text 
overlay parameters to streams and saved images.

For example, on the stream you could display just the date and time, but
on the saved files you could add more information such as the number
of changed pixels or different labels etc.

Another example might be that you could have no rotation on the stream
but still have saved images undergo rotation or vice versa.

Going even further, it might be possible to create extra stream types 
using different properties. For example the normal "stream" might have no
rotation but "stream2" may have 90 degree rotation.

The tradeoff of implementing this would be slightly more memory
consumption. In addition, if multiple different output image formats were 
being utilized simultaneously then the CPU would have to do the extra work
of manipulating each output image in multiple ways.

That being said, there would be no more CPU utilization during the "idle"
state or if only a single stream / picture output process was engaged.

### Conditional rotation.
Perhaps during high system CPU load you could turn off rotation? I'm
not sure if this would be appealing or not.




Motion
=============

## Description

Motion is a program that monitors the video signal from one or more cameras and
is able to detect if a significant part of the picture has changed. Or in other
words, it can detect motion.

## Documentation

The documentation for Motion is contained within the file motion_guide.html.

The offline version of this file is available in the **doc/motion** directory.  The
online version of the motion_guide.html file can be viewed [here](https://motion-project.github.io/motion_guide.html)

In addition to the detailed building instructions included within the guide, the
INSTALL file contains abbreviated building instructions.

## Resources

Please join the mailing list [here](https://lists.sourceforge.net/lists/listinfo/motion-user)

We prefer support through the mailing list because more people will have the benefit from the answers.
A archive of mailing list discussions can be viewed [here](https://sourceforge.net/p/motion/mailman/motion-user/)

## License

Motion is mainly distributed under the GNU GENERAL PUBLIC LICENSE (GPL) version 2 or later.
See the copyright file for a list of all the licensing terms of the various components of Motion.

The file CREDITS lists the many people who have contributed to Motion over the years.

## Contributing

Issues and Patches should be submitted via github and include detail descriptions
of the issue being addressed as well as any documentation updates that would be
needed with the change.

