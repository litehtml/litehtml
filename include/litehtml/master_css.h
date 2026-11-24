#ifndef LITEHTML_MASTER_CSS_H
#define LITEHTML_MASTER_CSS_H
namespace litehtml
{
    const char* const master_css = R"##(


html {
	display: block;
}

head {
	display: none
}

meta {
	display: none
}

title {
	display: none
}

link {
	display: none
}

style {
	display: none
}

script {
	display: none
}

body {
	display:block;
	margin:8px;
}

p {
	display:block;
	margin-top:1em;
	margin-bottom:1em;
}

b, strong {
	display:inline;
	font-weight:bold;
}

i, em, cite {
	display:inline;
	font-style:italic;
}

ins, u {
	text-decoration:underline
}

del, s, strike {
	text-decoration:line-through
}

center
{
	text-align:center;
	display:block;
}

a:link
{
	text-decoration: underline;
	color: #00f;
	cursor: pointer;
}

h1, h2, h3, h4, h5, h6, div {
	display:block;
}

h1 {
	font-weight:bold;
	margin-top:0.67em;
	margin-bottom:0.67em;
	font-size: 2em;
}

h2 {
	font-weight:bold;
	margin-top:0.83em;
	margin-bottom:0.83em;
	font-size: 1.5em;
}

h3 {
	font-weight:bold;
	margin-top:1em;
	margin-bottom:1em;
	font-size:1.17em;
}

h4 {
	font-weight:bold;
	margin-top:1.33em;
	margin-bottom:1.33em
}

h5 {
	font-weight:bold;
	margin-top:1.67em;
	margin-bottom:1.67em;
	font-size:.83em;
}

h6 {
	font-weight:bold;
	margin-top:2.33em;
	margin-bottom:2.33em;
	font-size:.67em;
}

br {
	display:inline-block;
}

br[clear="all"]
{
	clear:both;
}

br[clear="left"]
{
	clear:left;
}

br[clear="right"]
{
	clear:right;
}

span {
	display:inline
}

img {
	display: inline-block;
}

img[align="right"]
{
	float: right;
}

img[align="left"]
{
	float: left;
}

hr {
	display: block;
	margin-top: 0.5em;
	margin-bottom: 0.5em;
	margin-left: auto;
	margin-right: auto;
	border-style: inset;
	border-width: 1px
}


/***************** TABLES ********************/

table {
	display: table;
	border-collapse: separate;
	border-spacing: 2px;
	border-top-color:gray;
	border-left-color:gray;
	border-bottom-color:black;
	border-right-color:black;
	font-size: medium;
	font-weight: normal;
	font-style: normal;
	box-sizing: border-box;
}

tbody, tfoot, thead {
	display:table-row-group;
	vertical-align:middle;
}

colgroup {
	display: table-column-group;
}

col {
	display: table-column;
}

tr {
	display: table-row;
	vertical-align: inherit;
	border-color: inherit;
}

td, th {
	display: table-cell;
	vertical-align: inherit;
	border-width:1px;
	padding:1px;
}

th {
	font-weight: bold;
}

table[border] {
	border-style:solid;
}

table[border^="0"] {
	border-style:none;
}

table[border] td, table[border] th {
	border-style:solid;
	border-top-color:black;
	border-left-color:black;
	border-bottom-color:gray;
	border-right-color:gray;
}

table[border^="0"] td, table[border^="0"] th {
	border-style:none;
}

table[align=left] {
   float: left;
}

table[align=right] {
   float: right;
}

table[align=center] {
   margin-left: auto;
   margin-right: auto;
}

caption {
	display: table-caption;
}

td[nowrap], th[nowrap] {
	white-space:nowrap;
}

tt, code, kbd, samp {
	font-family: monospace
}

pre, xmp, plaintext, listing {
	display: block;
	font-family: monospace;
	white-space: pre;
	margin: 1em 0
}

/***************** LISTS ********************/

ul, menu, dir {
	display: block;
	list-style-type: disc;
	margin-top: 1em;
	margin-bottom: 1em;
	margin-left: 0;
	margin-right: 0;
	padding-left: 40px
}

ol {
	display: block;
	list-style-type: decimal;
	margin-top: 1em;
	margin-bottom: 1em;
	margin-left: 0;
	margin-right: 0;
	padding-left: 40px
}

li {
	display: list-item;
}

ul ul, ol ul {
	list-style-type: circle;
}

ol ol ul, ol ul ul, ul ol ul, ul ul ul {
	list-style-type: square;
}

dd {
	display: block;
	margin-left: 40px;
}

dl {
	display: block;
	margin-top: 1em;
	margin-bottom: 1em;
	margin-left: 0;
	margin-right: 0;
}

dt {
	display: block;
}

ol ul, ul ol, ul ul, ol ol {
	margin-top: 0;
	margin-bottom: 0
}

blockquote {
	display: block;
	margin-top: 1em;
	margin-bottom: 1em;
	margin-left: 40px;
	margin-right: 40px;
}

/*********** FORM ELEMENTS ************/

form {
	display: block;
	margin: 0;
}

fieldset {
	display: block;
	margin: 0 2px;
	padding: 0.35em 0.75em 0.625em;
	border: 1px solid #c0c0c0;
}

legend {
	display: block;
	padding: 0 2px;
}

label {
	display: inline;
}

option, optgroup {
	display: none;
}

input, textarea, keygen, select, button, isindex {
	margin: 0;
	color: initial;
	line-height: normal;
	text-transform: none;
	text-indent: 0;
	text-shadow: none;
	display: inline-block;
	vertical-align: middle;
}

input[type="hidden"] {
	display: none;
}

input {
	appearance: textfield;
	width: 20ch;
	height: 1.5em;
	padding: 0 2px;
	border: 1px solid #767676;
	border-radius: 2px;
	background-color: #fff;
	color: #000;
	cursor: text;
}

input[type="text"],
input[type="password"],
input[type="search"],
input[type="number"] {
	appearance: textfield;
	width: 20ch;
	height: 1.5em;
	padding: 0 2px;
	border: 1px solid #767676;
	border-radius: 2px;
	background-color: #fff;
	color: #000;
	cursor: text;
}

input[type="checkbox"],
input[type="radio"] {
	box-sizing: border-box;
	width: 13px;
	height: 13px;
	padding: 0;
	margin: 0 3px 0 0;
	border: 1px solid #767676;
	background-color: #fff;
	cursor: pointer;
}

input[type="checkbox"] {
	appearance: checkbox;
	border-radius: 2px;
}

input[type="radio"] {
	appearance: radio;
	border-radius: 50%;
}

input[type="checkbox"]:checked {
	border-color: #0b5ed7;
	background-color: #0b5ed7;
}

input[type="radio"]:checked {
	border-color: #0b5ed7;
}

button,
input[type="button"],
input[type="submit"],
input[type="reset"] {
	appearance: button;
	box-sizing: border-box;
	width: auto;
	height: auto;
	min-width: 4em;
	padding: 2px 10px;
	border: 1px solid #767676;
	border-radius: 2px;
	background-color: #e9e9e9;
	color: #000;
	cursor: pointer;
}

button {
	text-align: center;
}

input[type="image"] {
	display: inline-block;
	width: auto;
	height: auto;
	padding: 0;
	border: none;
	background-color: transparent;
	cursor: pointer;
}

textarea {
	appearance: textarea;
	box-sizing: border-box;
	width: 20ch;
	height: 3.7em;
	padding: 2px;
	border: 1px solid #767676;
	border-radius: 2px;
	background-color: #fff;
	color: #000;
	vertical-align: top;
	cursor: text;
}

select {
	appearance: menulist;
	box-sizing: border-box;
	min-width: 8em;
	height: 1.5em;
	padding: 0 2px;
	border: 1px solid #767676;
	border-radius: 2px;
	background-color: #fff;
	color: #000;
	cursor: pointer;
}

input:disabled,
textarea:disabled,
select:disabled,
button:disabled {
	color: #999;
	background-color: #f2f2f2;
	border-color: #c0c0c0;
}

:focus-visible {
	outline: auto 1px #E59700;
}

html:focus-visible,
body:focus-visible {
	outline: none;
}

embed:focus-visible,
iframe:focus-visible,
object:focus-visible {
	outline: none;
}

a:link:focus-visible {
	outline-offset: 1px;
}

input[type="checkbox"]:focus-visible,
input[type="radio"]:focus-visible {
	outline-offset: 2px;
}


article, aside, footer, header, hgroup, nav, section
{
	display: block;
}

sub {
	vertical-align: sub;
	font-size: smaller;
}

sup {
	vertical-align: super;
	font-size: smaller;
}

figure {
	display: block;
	margin-top: 1em;
	margin-bottom: 1em;
	margin-left: 40px;
	margin-right: 40px;
}

figcaption {
	display: block;
}

)##";
}
#endif // LITEHTML_MASTER_CSS_H
