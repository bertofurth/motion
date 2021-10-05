# Rotate image/Video only when saving or streaming

This fork is to implement the changes suggested in 

https://github.com/Motion-Project/motion/issues/1321

The suggestion was to only perform the CPU intensive
task of image rotation when images were actually
being viewed via a web stream or when they are being
saved.

This fork proposes a change to motion that causes motion
to store image files internally in the original / captured /
raw format and only perform rotation just before images
are written to files or streamed.

A brief test of these changes showed that when using
"rotation 90" motion's CPU utilization in the idle
state is reduced by about 20% on what it otherwise may have
been. By "idle state" I mean when no streams are active and
when no event triggered output files are being generated.

There are a number of other benefits which I'll outline
below but first it's important to note some of the drawbacks 
and changes that would have to be addressed.

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

TODO : Maybe have a "privacy_mask_source" and "mask_file_source"
to specify mask files that apply to the captured images.

### Debug streams  and images not rotated
The "source" (virgin) and "motion" streams are *not* rotated since
these types of images are now stored internally as unrotated.

It would not be difficult to rotate them however, in my view this
would defeat the purpose of these streams which, as I understand it,
are used for debugging. If they were rotated then they'd be less
"pure" and a problem with the rotation system would impact them.

This also goes for "picture_output_motion" and "movie_output_motion"

### Error messages and debug text overlays
Error messages or text applied to images when using log level "DBG"
are applied to images *before* they are rotated. This means that if
an image is rotated after one of these types of text are overlaid
the text will also get rotated.


### "netcam_hires" streams
Text overlays *will* be applied to "netcam_hires" stream outputs but
obviously not for the video file when "movie_passthrough" is specified.



Possible future work:
One potential extra feature that could be easily introduced with this
change is that you could optionally apply separate rotation and text 
overlay parameters to streams and saved images.

For example on the stream you could display just the date, but on 
the saved files you could have much more information such as the number
of changed pixels, the number of events gone by etc.

Futhermore you could have no rotation on the stream but
save images with a particular rotation.

Another idea is that you could have extra stream names that correspond to the desired rotation. For example

http://192.0.2.99:8081/101/stream90   (Rotates stream by 90 deg)

http://192.0.2.99:8081/101/stream180  (Rotates stream by 180 deg)

http://192.0.2.99:8081/101/streamflipv  (Flips stream vertically)


Naturally the tradeoff is that if multiple different types of image manipulation parameters are being utilized at (i.e. someone viewing two different styles of stream at once) then the CPU will have to go to the extra effort of performing that extra image processing.




![image](https://user-images.githubusercontent.com/53927348/135963949-56e0dee9-8d27-47b7-8dcf-14d24b580062.png)



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

