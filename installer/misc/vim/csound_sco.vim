" Vim syntax file
" Language:     Csound score file
" Maintainer:   Istvan Varga
" Last change:  Mar 1 2006

syn	clear

set	nocompatible
set	autoindent
set	noignorecase
set	showmatch

" score statements (f, t, i etc.)

syn	match	Statement	"^[ \t]*[[:alpha:]]"

" comments

syn	region	Comment	start="/\*" end="\*/"
syn	match	Comment ";.*$"

" backslash

syn	match	Special	"\\[[:space:]]*$"

" constants

syn	match	Constant	"\<[[:digit:]]\+\(\.[[:digit:]]\+\)\{,1\}\([Ee][+-]\{,1\}[[:digit:]]\+\)\{,1\}\>"
syn	region	Constant	start="\"" end="\""

" include files and macros

syn	match	PreProc	"^[[:space:]]*#"
syn	match	PreProc	"#[[:space:]]*$"
syn	match	PreProc	"^[[:space:]]*#include\>"
syn	match	PreProc	"^[[:space:]]*#define\>"
syn	match	PreProc	"^[[:space:]]*#define[^#]*#"
syn	match	PreProc	"^[[:space:]]*#undef.*$"
syn	match	PreProc	"\$[[:alpha:]][[:alnum:]_]*\(([^()]*)\)\{,1\}\.\{,1\}"

" some useful keyboard shortcuts

" ``#i: include file

imap	``#i	#include ""<CR><UP><HOME><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT>

" ``#d: define macro

imap	``#d	#define  # #<CR><UP><HOME><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT>

" ``#u: undefine macro

imap	``#u	#undef <CR><UP><HOME><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT><RIGHT>

" ``f: insert dummy score

imap	``f	<CR>f 0 3600<CR>e<CR>

