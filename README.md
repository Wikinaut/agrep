<img src="https://raw.github.com/Wikinaut/agrep/master/resources/agrep.gif">

AGREP - an approximate GREP. 

Fast searching files for a string or regular expression, with approximate matching capabilities and user-definable records. 

Developed 1989-1991 by Udi Manber, Sun Wu et al. at the University of Arizona.

For Glimpse and WebGlimpse - AGREP is an essential part of them - see

* https://github.com/gvelez17/glimpse
* https://github.com/gvelez17/webglimpse


Usage
=====

Type

    agrep

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
-n  print line numbers of matches          -q  print buffer byte offsets
-p  supersequence search                   -CP 850|437 set codepage
-r  recurse subdirectories (UNIX style)    -s  silent
-t  for use when delimiter is at the end of records
-v  output those records without matches   -V[012345V] version / verbose more
-w  pattern has to match as a word: "win" will not match "wind"
-u  suppress record output                 -x  pattern must match a whole line
-y  suppress the prompt when used with -B best match option
@listfile  use the filenames in listfile                              <1>23456Q
```

Branches
========

The present repository contains three different branches:

* **master**: agrep based on agrep 3.0, ported to OS/2, DOS and Windows in the 90ies, and backported to LINUX (the present version you are visiting)
* **agrep3.0-as-found-in-glimpse4.18.6-20130216**: agrep 3.0 as it was found in the glimpse software
* **agrep2.04**: the first published and original agrep version


Installation
============

```
git clone git@github.com:Wikinaut/agrep.git
cd agrep
make
```


Algorithms
==========

* [Wu, S., Manber, U.: "Agrep - A Fast Approximate Pattern-Matching Tool", 1992.](http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.48.8488)
* [Wu, S., Manber, U.: "Fast Text Searching With Errors", 1991.](http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.20.8854)
* see [agrep.algorithms](https://github.com/Wikinaut/agrep/blob/master/agrep.algorithms)
* see [docs](https://github.com/Wikinaut/agrep/blob/master/docs)
* see [readme](https://github.com/Wikinaut/agrep/blob/master/docs/README)


COPYRIGHT
=========

* see [COPYRIGHT](https://github.com/Wikinaut/agrep/blob/master/COPYRIGHT)


As of Sept 18, 2014, Webglimpse, Glimpse and Agrep are available under
the ISC open source license, thanks to the
University of Arizona Office of Technology Transfer and all the developers,
who were more than happy to release it.

Sources:
http://webglimpse.net/sublicensing/licensing.html
http://opensource.org/licenses/ISC

Anyone distributing the AGREP code should include the following license
which is applicable since September 2014:

Copyright 1996, Arizona Board of Regents
on behalf of The University of Arizona.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. 

IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
OF THIS SOFTWARE.


Contributors
============

* see [contribution.list](https://github.com/Wikinaut/agrep/blob/master/contribution.list)


History
=======

* see [agrep.chronicle](https://github.com/Wikinaut/agrep/blob/master/agrep.chronicle)


Alternatives to AGREP
=====================

[Alternatives](http://www.tgries.de/agrep):
* TRE is a lightweight, robust, and efficient POSIX compliant regexp matching library with some exciting features such as approximate (fuzzy) matching. 
* AGREPY: Python port of agrep string matching with errors
* The bitap library , another new and fresh implementation of the bitap algorithm. Windows - C - Cygwin
* PERL module String:Approx. Perl extension for approximate matching (fuzzy matching) by Jarkko Hietaniemi, Finland
* ugrep https://github.com/Genivia/ugrep

Further stuff with the same name (agrep)
========================================

* [aGrep](https://play.google.com/store/apps/details?id=jp.sblo.pandora.aGrep&hl=de), published in 2012, is an Android implementation of ```grep``` (but not ```agrep```).


Homepage and references
=======================

* http://www.tgries.de/agrep
* http://webglimpse.net
* https://en.wikipedia.org/wiki/Agrep
* [Wu, S., Manber, U.: "Agrep - A Fast Approximate Pattern-Matching Tool", 1992.](http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.48.8488)
* [Wu, S., Manber, U.: "Fast Text Searching With Errors", 1991.](http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.20.8854)
