/*
 csanalye.c:

 Copyright (C) 2013 Steven Yi

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
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307 USA
 */

#include "csound.h"

extern void     print_tree(CSOUND *, char *, TREE *);

void header() {
    printf("csanalyze - developer utility program to analyze "
           "Csound's compiler phases\n\n");
}

void usage() {

}



int main(int argc, char** argv) {
    CSOUND* csound;

    header();

    if(argc != 2) {
        usage();
        return 1;
    }

    printf("Input File: %s", argv[1]);

    csound = csoundCreate(NULL);

    printf("csound=%p\n", csound);
    return 0;
}
