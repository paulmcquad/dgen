Name
	scalerx - Scale a .PNG image using the reference implementation of the Scale effects

Synopsis
	:scalerx [-k N] [-w] [-r N] input.png output.png

Description
	Scale2x, Scale3x and Scale4x are real-time graphics effects
	able to increase the size of small bitmaps guessing the
	missing pixels without blurring the images.

	They were originally developed for the AdvanceMAME project
	in the year 2001 to improve the quality of old games running
	at low video resolutions.

	The specification of the algorithm and more details are at:

		+http://www.scale2x.it

	These command line tools read a .PNG file and write another
	image with the effects applied.

	The reference implementation of the Scale effects is used.

Options
	-k N, --scale N
		Select the scale factor. Available values are 2, 2x3,
		2x4, 3, and 4.

	-w, --wrap
		Compute the image border for a wraparound effect.

	-r N
		Select a specific revision of the effect.
		The default is the 'scaleX' algorithm, selected with 'x' or '1'.
		To select the new 'scaleK' algorithm use 'k' or '2'.
		To scale an image without any effect use the value '0'.
		Other numbers are alternative and not documented effects.
		Refers at the source implementation for more details.

Copyright
	This file is Copyright (C) 2003, 2004 Andrea Mazzoleni

