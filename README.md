Motion with "Just in Time" Rotation
===================================

TLDNR : These proposed changes can save up to about 25%
CPU load in the idle state when 90 or 270 degree rotation 
is applied for images.

The original motion README.md file is below
these notes.

# Rotate images / video only when saving or streaming
This fork of motion is a proposed method of implementing
the changes suggested in 

https://github.com/Motion-Project/motion/issues/1321

The suggestion was to only perform the CPU intensive
task of image rotation only when images were actually
being viewed via a web stream or when they are being
saved.

This fork proposes a change where motion will only perform
rotation and text annotation of images if they are actually
being saved to a file, streamed or sent to a loopback device.

The main change this fork makes is that image files are
internally stored in the original, as captured orientation.

A brief test of these changes using "rotation 90" reduced
motion's CPU utilization in the "idle state" by about 20% of
what it otherwise may have been when using a 640x480 stream
at 4 frames per second. See the "Performance Tests" section
for more details.

The tradeoff is a slight memory usage increase due to the
binary executable being slightly larger and needing to reserve
some extra memory to cater for rotated images.

In addition CPU utilization may be slightly increased compared 
to the original version of motion where "motion" debugging images
were being saved and/or viewed in a stream at the same time as
normal images were being saved and/or viewed in a stream. My 
understanding is that this type of scenario only typically occurs
during debugging and initial setup of motion.

See below for a discussion of issues and benefits of these
proposed changes.

## The changes
### Images are stored internally as unrotated and unannotated
The basis of the new proposed functionality is that images are
stored internally within motion in their unrotated, and largely 
unprocessed form.

By not rotating images until they actually need to be rotated
we save a measurable amount of CPU resources, particuarly when
90 or 270 degree rotation is used.

Some further miniscule CPU savings are made by not performing
"flip on axis" or text annoation of images until they are needed.

### Rotation buffers consolidated with common buffer
Originally, the rotation functions had their own dedicated buffers
to perform 90 and 270 degree image rotation. These have been removed
and the common image buffer (cnt->imgs.common_buffer) is now used for
rotation. This saves about one raw image's worth of memory from being
allocated per camera.

### PGM mask files (mask_file and mask_privacy)
Since images are stored internally as unrotated, masks need to be
applied to images using the source dimensions, not the displayed 
or post-rotation dimensions.

Since "mask_file" and "mask_privacy" are documented as needing to
be the same dimensions as the output file we now have to internally
"unrotate" the mask files.

It could potentially be better going forward to specify that masks
apply to the original captured image dimentions because this way a
user can arbitrarily change the rotation or the flip axis of the
displayed image without having to worry about making corresponding 
changes to the privacy mask.

For this reason there are two new boolean (on/off) configuration file
options. 

#### privacy_mask_rotated 
Type: Boolean
Range / Valid values: on, off
Default: on

If set to "on" then the supplied privacy_mask pgm image is oriented
to match the dimensions and rotation of the output image.

If set to "off" then the supplied privacy_mask pgm image is applied to
the unrotated and unflipped original captured image from the source.

#### mask_file_rotated 
Type: Boolean
Range / Valid values: on, off
Default: on

If set to "on" then the supplied mask_file pgm image is oriented
to match the dimensions and rotation of the output image.

If set to "off" then the supplied mask_file pgm image is applied to the
unrotated and unflipped original captured image from the source.

### Rotation of "motion" images and movies
By default "motion" images generated by the "picture_output_motion on"
configuration option or viewed in the "motion" stream will be rotated
in the same way as normal output pictures. 

The problem is that performing this extra rotation will potentially add
CPU load to the system in the case where *both* normal and "motion"
debugging images are being generated because it will mean that both
types of images will need to be rotated individually.

There is a new proposed configuration option to disable the rotation of
"motion" debugging images in order to save CPU cycles.

#### picture_output_motion_rotated
Type: Boolean
Range / Valid values: on, off
Default: on  (*Should this be off? Thoughts?*)

This setting applies to motion debugging images generated by 
"picture_output_motion", "movie_output_motion", to the "motion" stream
and "video_pipe_motion".

If set to "on" then motion images will have "rotate" and "flip_axis"
parameters applied as per normal pictures. Rotation, particularly 90 and
270 degree rotation, can consume significant CPU resources. This feature
also consumes a small amount of extra memory.

By setting this feature to "off", motion debugging images will not be 
rotated. This will save CPU and memory resources while motion debugging
images are being saved or viewed in a stream.

## Issues 
### "source" stream is not rotated
The "source" (virgin) stream is not rotated since these
these types of images are now stored internally as unrotated.

It would not be difficult to add a new type of stream (say
"source-rot") that displayed the rotated stream if this was
required however this would seem to be what the normal
image stream is doing already.

### "motion" stream and picture rotation 
Since images are now stored unrotated, "motion" images, which are
used for debugging, have to have extra CPU resources applied to them
in order to rotate them when they are being generated for a stream or
file output.

This causes these proposed changes use more CPU than originally in
the case where the "picture_output_motion" or motion streaming feature
was in use and rotation was being applied.

See the section below on "Rotation of "motion" images and movies"
for a proposed workaround using a new configuration option.

### Error messages and debug text overlays
Embedded error messages such as "CONNECTION TO CAMERA LOST" or text
debugs shown when using log level "DBG" are applied to images *before*
they are rotated. This means that if an image is subsequently rotated 
after one of these types of text are overlaid then that text will also
get rotated. 

This is fixable but the layout of the error messages might need to be
slightly changed.

### Automatic mask file generator
If motion is unable to read a configured pgm mask file then it will 
automatically generate one based on the dimensions of a
generated image. (See put_fixed_mask() for details)

With these changes the generated file will be based on the
captured image dimentions rather than the generated image
dimentions. This might be an issue where 90 or 270 degree
rotation has been configured.

## Performance Tests
Motion was tested on a Raspberry Pi 4B with an attached USB
camera supplying a raw (uncompressed) 640x480 stream at 10
frames per second.

"picture_output on" and "movie_output off" were configured 
so that pictures but not movies would be saved.

The following characterics were applied to tests.

**Scenario** - Either **Idle** indicating no motion was being
detected or **emulate motion** indicating that 
"emulate_motion on" is configured to simulate motion detection
all the time. The scene being monitored was static. 

**Rot** - The "rotate" parameter was set to either 0 or 90.

**Motion Output** - The "picture_output_motion" is set to either
on or off to generate motion debugging images.

**Stream** - One user is viewing the normal stream

**Motion Stream** - One user is viewing the motion stream

CPU time was measured by starting motion, giving the program approximately
30 seconds to initialize and stablize, then recording cpu seconds utilized 
over 600 seconds of run time using the output of the following "bash"
command run in a different terminal window on the same machine.

    ps -o etime,cputime $(pidof motion) ; sleep 600; ps -o etime,cputime $(pidof motion)
    

### Original motion

| Scenario          | Rot  | Motion | Stream | Motion | CPU   |
|                   |      | Output |        | Stream | Time  |      
|-------------------|------|--------|--------|--------|-------|
| Idle              |  0   | off    | no     | no     |    93 |
| Idle              | 90   | off    | no     | no     |   105 |   ?
| Idle              |  0   | off    | yes    | no     |    98 |
| Idle              | 90   | off    | yes    | no     |   111 |
| Idle              |  0   | off    | no     | yes    |    93 |
| Idle              | 90   | off    | no     | yes    |   111 |
| Idle              |  0   | off    | yes    | yes    |   103 |
| Idle              | 90   | off    | yes    | yes    |   114 |
| Emulate Motion    |  0   | off    | no     | no     |    92 |
| Emulate Motion    | 90   | off    | no     | no     |   100 |
| Emulate Motion    |  0   | on     | no     | no     |   100 |
| Emulate Motion    | 90   | on     | no     | no     |   116 |
| Emulate Motion    | 90   | on     | yes    | yes    |   180 |




### Just in Time rotation motion

| Scenario          | Rot  | Motion | Stream | Motion | CPU   |
|                   |      | Output |        | Stream | Time  |      
|-------------------|------|--------|--------|--------|-------|
| Idle              |  0   | off    | no     | no     |       |
| Idle              | 90   | off    | no     | no     |       |   
| Idle              |  0   | off    | yes    | no     |       |
| Idle              | 90   | off    | yes    | no     |       |
| Idle              |  0   | off    | no     | yes    |       |
| Idle              | 90   | off    | no     | yes    |       |
| Idle              |  0   | off    | yes    | yes    |       |
| Idle              | 90   | off    | yes    | yes    |       |
| Emulate Motion    |  0   | off    | no     | no     |       |
| Emulate Motion    | 90   | off    | no     | no     |       |
| Emulate Motion    |  0   | on     | no     | no     |       |
| Emulate Motion    | 90   | on     | no     | no     |       |
| Emulate Motion    | 90   | on     | yes    | yes    |       |





 




### "Just In Time" rotation with "picture_output_motion_rotated on"



### "Just In Time" rotation with "picture_output_motion_rotated off"






## Future work
The proposed changes might make it easy to implement some of these
other new functionaly which might (or might not) be useful.

### Different text/rotation for streams and saved images
Users could optionally apply separate rotation and text 
overlay parameters to streams and saved images.

For example, on the stream you could display just the date and time, but
on the saved files you could add more information such as the number
of changed pixels or different labels etc.

Another example might be that you could have no rotation on the stream
but still have saved images undergo rotation or vice versa.

The tradeoff would be that extra CPU resources would have to
be utilized if both the stream were active and pictures were
being saved with different parameters at the same time.

That being said, there would be no more CPU utilization during the "idle"
state or if only a single stream / picture output process was engaged.

### Extra stream types
As per the previous idea, it might be possible to create extra
stream types using different properties. For example the normal 
stream might have no rotation but "stream2" may have 90 degree rotation.




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

