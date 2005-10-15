Here are the files from Hans showing how to build an OS X installer package
and make a DMG disk image of the package from a command-line script.  His files
were for Csound 4.23 and his email about them is below.  This should hopefully
help someone get started on this task for Csound 5.

The package-building commands are in the Makefile.macosx.

I would also suggest including a nice README intro to Csound in the DMG 
explaining how to install and configure.  (Links to the manual might be nice
too :)

-- Anthony Kozar  


Date: Tue, 14 Oct 2003 12:20:47 -0400
Subject: [CSOUND-DEV:3135] MacOS X installer maker
From: Hans-Christoph Steiner <hans@eds.org>
To: Csound Developers Discussion List <csound-dev@eartha.mills.edu>

I have written up a section for the Makefile which creates a MacOS X  
installer for csound just by typing 'make darwin_pkg'.  You can make a  
distributable .dmg file by typing 'make dmg' afterwards.  There are  
three files involved: csound.info (the .pkg info), mkdmg (script to  
make the .dmg), and Makefile.macosx.  These three files are all  
attached.
For Makefile.macosx, all of the pkg building stuff is appended to the  
end of the file.  I would love to see these files added to the CVS.
