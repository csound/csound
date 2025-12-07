/*
    new_orc_parser.h: parser functions from bison

    Copyright (C) 2007, 2017 by Steven Yi and John ffitch

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

void csound_orcrestart(FILE*, void *);
void print_csound_predata(void *);
int32_t csound_prelex_init(void *);
void csound_preset_extra(void *, void *);
int32_t csound_prelex(CSOUND*, void*);
int32_t csound_prelex_destroy(void *);
struct yy_buffer_state *csound_orc_scan_buffer(const char *, size_t, void*);
int csound_orcparse(PARSE_PARM *, void *, CSOUND*, TREE**);
int32_t csound_orclex_init(void *);
void csound_orcset_extra(void *, void *);
void csound_orcset_lineno(int32_t,  void*);
int32_t csound_orclex_destroy(void *);
TREE* csound_orc_optimize(CSOUND *, TREE *);

