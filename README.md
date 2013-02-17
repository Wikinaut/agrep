<img src="https://raw.github.com/Wikinaut/agrep/master/resources/agrep.gif">

agrep
=====

AGREP -- an approximate GREP. 

Fast searching files for a string or regular expression, with approximate matching capabilities and user-definable records. 

Developed 1989-1991 by Udi Manber, Sun Wu et al. at the University of Arizona.

Usage
=====

Type

```$ agrep```

to get the six built-in help pages.

```
           Approximate Pattern Matching GREP -- Get Regular Expression
Usage:
AGREP [-#cdehi[a|#]klnprstvwxyABDGIRS] [-f patternfile] [-H dir] pattern [files]
-#  find matches with at most # errors     -A  always output filenames
-b  print byte offset of match
-c  output the number of matched records   -B  find best match to the pattern
-d  define record delimiter                -Dk deletion cost is k
-e  for use when pattern begins with -     -G  output the files with a match
-f  name of file containing patterns       -Ik insertion cost is k
-h  do not display file names              -Sk substitution cost is k
-i  case-insensitive search; ISO <> ASCII  -ia ISO chars mapped to lower ASCII
-i# digits-match-digits, letters-letters   -i0 case-sensitive search
-k  treat pattern literally - no meta-characters
-l  output the names of files that contain a match
-n  print line numbers of matches  -q print buffer byte offsets
-p  supersequence search                   -CP 850|437 set codepage
-r  recurse subdirectories (UNIX style)    -s silent
-t  for use when delimiter is at the end of records
-v  output those records without matches   -V[012345V] version / verbose more
-w  pattern has to match as a word: "win" will not match "wind"
-u  unterdruecke record output             -x  pattern must match a whole line
-y  suppresses the prompt when used with -B best match option
@listfile  use the filenames in listfile                              <1>23456Q
```


Installation
============

```$ make```


Algorithms
==========

* see [agrep.algorithms]((https://github.com/Wikinaut/agrep/blob/master/agrep.algorithms))
* see [/docs](https://github.com/Wikinaut/agrep/blob/master/docs)
* see [readme](https://github.com/Wikinaut/agrep/blob/master/readme)


COPYRIGHT
=========

* see [COPYRIGHT](https://github.com/Wikinaut/agrep/blob/master/COPYRIGHT)

This material was developed by Sun Wu, Udi Manber and Burra Gopal
at the University of Arizona, Department of Computer Science.
Permission is granted to copy this software, to redistribute it
on a nonprofit basis, and to use it for any purpose, subject to
the following restrictions and understandings.

1. Any copy made of this software must include this copyright notice
in full.

2. All materials developed as a consequence of the use of this
software shall duly acknowledge such use, in accordance with the usual
standards of acknowledging credit in academic research.

3. The authors have made no warranty or representation that the
operation of this software will be error-free or suitable for any
application, and they are under under no obligation to provide any
services, by way of maintenance, update, or otherwise. The software
is an experimental prototype offered on an as-is basis.

4. Redistribution for profit requires the express, written permission
of the authors.


Contributors
============

* see [contribution.list](https://github.com/Wikinaut/agrep/blob/master/contribution.list)


History
=======

* see [agrep.chronicle](https://github.com/Wikinaut/agrep/blob/master/agrep.chronicle)


Homepage
========

* http://www.tgries.de/agrep
* http://webglimpse.net
