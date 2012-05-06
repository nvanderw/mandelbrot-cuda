# Simple utility to write out a colors.bin file, which is a binary packed
# with color information for the mandelbrot utility. Such a file consists of
# sequential bytes of color data in the format RGBRGBRGB...

# This utility takes as input an integral number of colors to generate and a
# path to an output file and generates packed color data as described above.
# The colors it generates are evenly spaced in hue, have constant 

import colorsys
import sys
import argparse


# Some defaults, in case command-line options aren't passed

parser = argparse.ArgumentParser(description="Generate colors of evenly-spaced "
         + "hues")

parser.add_argument("-o", metavar="out", nargs=1, dest="out",
                    type=argparse.FileType("wb"), required=True)

parser.add_argument("-n", metavar="numcolors", nargs=1, dest="numcolors",
                    type=int, default=[10000])

parser.add_argument("-s", metavar="saturation", nargs=1, dest="saturation",
                    type=float, default=[0.9])

parser.add_argument("-v", metavar="value", nargs=1, dest="value", type=float,
                    default=[1.0])

args = parser.parse_args(sys.argv[1:])

for i in xrange(args.numcolors[0]):
    for comp in colorsys.hsv_to_rgb((1.0*i)/args.numcolors[0],
                                    args.saturation[0], args.value[0]):
        

        args.out[0].write(chr(int(comp*255)))
