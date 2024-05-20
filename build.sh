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
    bash build_macos.sh $1
    # bash engine_src/macos/buildtests.sh $1
elif [[ $PLATFORM = "Linux" ]]; then
    bash engine_src/linux/build.sh $1
elif [[ $PLATFORM = "MINGW64_NT-10.0-19045" ]]; then
    bash build_windows.sh $1
else
    echo "Unknown operating system: $PLATFORM"
    echo "Please add it in build.sh so it points to the correct OS build script"
    echo "You can also call build_windows.sh, build_macos.sh or build_linux.sh directly."
fi

