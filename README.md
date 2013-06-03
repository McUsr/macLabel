macLabel
========

macLabel, lets you list, set, test for, and set FinderLabel colors by name or number from the command line.
maclabel: version 1.0 Copyright © 2013 McUsr and put into Public Domain
  under Gnu GPL 2.0
Usage: macLabel [options]  [color] [1 to n ..file arguments or from stdin
       specified by posix path, one file on each line.]
Options
-------
  macLabel [ - -huVclastwvnd ]
  macLabel [ --help,--usage,--copyright,--version,--list,--all,--set
             --test,--which,--verbose, --number, --delimiter ]
Details
-------
  Label color can be set, returned or tested for.  Colors are numbers
  by -n or English  as -v or by default. Another delimiter than '\t' (tab)
  can be chosen to separate files from colors/numbers. Only files on a HFS+
  filesystem, can be operated upon. And the files must of course exist.

  -l  [file1 ..file1] list files and their label color.
  -a  [file1 ..file1] list all files and their label color (none).
  -s  [color] [file1 ..file1] set label to color. 
      -sn signalizes that color is set by number.
	  
  -t [color] [file 1 .. file n] test for label of file.
     exit code is zero if single file and success.
    -v output to stdout automatically set.
	-n returns the color as number.
	 
  -w [color] [file 1 ..file n] : returns files with a given color.
     exit code is colorcode if single file.
     -v is automatically set.
     -n returns the color as number.
	 
  -n [number] for color with the -s or --set option. (See below).

Valid colors and numbers representing them
-------------------------------------------
  0 None   1 Grey
  2 Green  3 Purple
  4 Blue   5 Yellow
  6 Red    7 Orange

Examples of usage
-----------------
  macLabel -w [file 1 .. file n]
  Returns [file color] to stdout.

  macLabel -wn [file 1 .. file n]
  Returns [file nr] to stdout.
  
  macLabel -w [file 1]
  Returns the colornumber as exitcode.

  macLabel -sn 6 [file 1]
  Sets the Finder Label of file 1 to red.

  macLabel -s red [file 1]
  Sets the Finder Label of file 1 to red.

  macLabel -t red [file 1 .. file n]
  Returns 0 if red is the label of file 1.
  Sends [file color] to stdout when  more than one file.
  
  macLabel -tn red [file 1 .. file n]
  Sends [file color-number] to stdout when more than one.
