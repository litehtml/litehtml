#What is litehtml?

**litehtml** is the lightweight HTML redering engine with CSS2/CSS3 support. Note, **litehtml** itself does not draw any text, pictures or other graphics and **litehtml** does not depend of any image/draw/font library. You are free to use any library to draw images, fonts and any other graphics. **litehtml** just parses HTML/CSS and places the HTML elements into right position (renders HTML). To draw the html elemens you have to implement the simple callback interface [document_container](https://github.com/litehtml/litehtml/wiki/document_container). This interface is really simply, check it! Note, the [document_container](https://github.com/litehtml/litehtml/wiki/document_container) implementation is required to render HTML correctly. 

#Where litehtml can be used

**litehtml** can be used when you need to show the html-formated texts or even to create a mini-browser, but the using full-featured html engine is not possible. Usually you don't need something like WebKit to show some html tooltips or html-formated text, **litehtml** is much better for these.

##HTML Parser

**litehtml** uses the [gumbo-parser](https://github.com/google/gumbo-parser) to parse HTML. Gumbo is an implementation of the HTML5 parsing algorithm implemented as a pure C99 library with no outside dependencies. It's designed to serve as a building block for other tools and libraries such as linters, validators, templating languages, and refactoring and analysis tools.

##Compatibility

**litehtml** is compatible with any platform suported C++ and STL. For Windows the MS Visual Studio 2013 is recommended. **litehtml** supports both utf-8 and unicode strings on Windows and utf-8 strings on Linux.

##Support for HTML and CSS standards

**litehtml** is not compatible with HTML/CSS standards. Although the gumbo-parser provides a good HTML5 parsing, **litehtml** does not supports some tags (frames, iframes, forms). But the most tags are supported. Also **litehtml** supports the most CSS2/3 properties, but not all. You can find the list of supported properties in  [this table](https://docs.google.com/spreadsheet/ccc?key=0AvHXl5n24PuhdHdELUdhaUl4OGlncXhDcDJuM1JpMnc&usp=sharing).

**litehtml** is in active developing and the support of new html/css features will be released in the future versions.

##Testing litehtml

You can [download the simple browser](http://www.litehtml.com/download.html) (**litebrowser**) to test the **litehtml** rendering engine. 

The litebrowser source codes are available on GitHub:
  * [For Windows](https://github.com/tordex/litebrowser)
  * [For Linux](https://github.com/tordex/litebrowser-linux)

##License

**litehtml** is distributed under [New BSD License](http://opensource.org/licenses/BSD-3-Clause).
The **gumbo-parser** is disributed under [Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0)

##Support litehtml project

If you think litehtml is amazing please consider a small donation:

[ ![PayPal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif) ](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=UHBQG6EAFCRBA)

Bitcoin: **1CS1174GVSLbP33TBp8RFwqPS6KmQK6kLY**

![BitCoin](https://www.tordex.com/assets/images/litehtml-bitcoin.png)
