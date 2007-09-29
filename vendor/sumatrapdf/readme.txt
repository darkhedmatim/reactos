Sumatra PDF (GNU GPL2) is composed from four parts: 

* an improved version of mupdf pdf library (http://ccxvii.net/apparition/). 
   tt's the default engine in Sumatra since version 0.4, since it's much faster than poppler. 
* poppler-kjk - my improved version of poppler pdf library (http://poppler.freedesktop.org/)
   poppler in turn, is a fork of xpdf (http://www.foolabs.com/xpdf/)
* baseutils - a library of small, hopefully reusable components like string handling etc. 
   Windows UI code that uses poppler for parsing and rendering 
* upcoming v0.8 relies on Google breakpad code for bug/crash reporting


http://blog.kowalczyk.info/software/sumatrapdf/
http://code.google.com/p/sumatrapdf/

SVN: http://sumatrapdf.googlecode.com/svn/trunk/



Klemens Friedl, 2007-09-29