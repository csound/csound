aclocal $ACLOCAL_FLAGS
automake -a $am_opt
autoconf
./configure --enable-maintainer-mode "$@"

echo "Now type 'make' to compile"
