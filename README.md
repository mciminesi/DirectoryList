# myls.c
## Description
Implementation of the ls command with selected options.
This program is designed to catalog the contents of the current directory.
Program can be compiled using the <make> command from the included makefile.

## Features:
  * Dynamically resizing multi-column format.
  * Runs on multiple command line directory arguments, including:
    ** The root directory ("/")
    ** Named directories ("/users/")
    ** Current or previous directory (either with no argments or by using ".")
    ** Directory relative to the current or previous ("./users/" or "../users/")

## Options included:
  * a   Desc: do not hide entries starting with .
  * A   Desc: do not list implied . and ..
  * F   Desc: append indicator (one of *=@|/) to entries
  * g   Desc: like -l, but do not list owner
  * G   Desc: inhibit display of group information
  * i   Desc: print index number of each file
  * l   Desc: use a long listing format
  * n   Desc: like -l, but list numeric UIDs and GIDs
  * o   Desc: like -l, but do not list group information
  * p   Desc: append indicator (one of /=@|) to entries
  * Q   Desc: enclose entry names in double quotes
  * r   Desc: reverse order while sorting
  * s   Desc: print size of each file, in blocks
  * w   Desc: assume screen width instead of current value
             Entering a value o 0 for this option will result in all output on a single line.

All options interact the same as they do in ls.