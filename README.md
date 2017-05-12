# myls.c
## Description
Implementation of the ls command with selected options.
This program is designed to catalog the contents of the current directory.
Program can be compiled using the <make> command from the included makefile.

## Features:
  * Dynamically resizing multi-column format.
  * Runs on multiple command line directory arguments, including:
    1. The root directory ("/")
    2. Named directories ("/users/")
    3. Current or previous directory (either with no argments or by using ".")
    4. Directory relative to the current or previous ("./users/" or "../users/")

## Options included:
  * a: do not hide entries starting with .
  * A: do not list implied . and ..
  * F: append indicator (one of *=@|/) to entries
  * g: like -l, but do not list owner
  * G: inhibit display of group information
  * i: print index number of each file
  * l: use a long listing format
  * n: like -l, but list numeric UIDs and GIDs
  * o: like -l, but do not list group information
  * p: append indicator (one of /=@|) to entries
  * Q: enclose entry names in double quotes
  * r: reverse order while sorting
  * s: print size of each file, in blocks
  * w: assume screen width instead of current value
             Entering a value o 0 for this option will result in all output on a single line.

All options interact the same as they do in ls.