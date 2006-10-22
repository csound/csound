#!/bin/sh
swig -cffi -module csound -v -extern-all -nocwrap -generate-typedef -ID:/utah/opt/swigwin-1.3.29/lib/cffi -I. -I.. -ID:/utah/home/mkg/projects/csoundd/H D:/utah/home/mkg/projects/csoundd/H/csound.h 
swig -cffi -module filebuilding -v -extern-all -nocwrap -generate-typedef -ID:/utah/opt/swigwin-1.3.29/lib/cffi -I. -I.. -ID:/utah/home/mkg/projects/csoundd/H D:/utah/home/mkg/projects/csoundd/interfaces/filebuilding.h
