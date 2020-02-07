sh beta-build.sh $1
cp beta/installer/csound$1beta-OSX-universal.pkg builds/macos/
cd builds/macos
git add csound$1beta-OSX-universal.pkg
git commit -am "overnight build"
git push
