PLATFORM="$(uname -s)"

if test -f "engine_src/shared/debigulator/src/decode_png.c";
then
echo "."
else
echo "Debigulator folder empty... Forgot to clone with submodules again?"
echo "reminder: to download the submodule, try git command:"
echo "git submodule update --init"
exit 0
fi

if [[ $PLATFORM = "Darwin" ]]; then
    bash engine_src/macos/build.sh $1
    # bash engine_src/macos/buildtests.sh $1
elif [[ $PLATFORM = "Linux" ]]; then
    bash engine_src/linux/build.sh $1
else
    echo "Unknown platform, please add in build.sh"
fi

