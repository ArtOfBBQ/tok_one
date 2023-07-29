# echo "Deleting buggy, useless, resource intensive and in all other ways awful Apple malware..."
# rm -r /Users/jellevandeneynde/Library/Developer/Xcode/DerivedData

if test -f "engine_src/shared/debigulator/src/decode_png.c";
then
echo "."
else
echo "Debigulator folder empty... Forgot to clone with submodules again?"
echo "reminder: to download the submodule, try git command:"
echo "git submodule update --init"
exit 0
fi

# bash engine_src/linux/build.sh
bash engine_src/macos/build.sh
# bash engine_src/macos/buildtests.sh
# bash engine_src/ios/build.sh

