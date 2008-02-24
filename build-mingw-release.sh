source ./debugd.sh
./cleand.sh
buildd.sh buildRelease=1 noDebug=1
doxygen
./strip.sh
