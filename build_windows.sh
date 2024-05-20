APP_NAME="tok_one"

# C:/"Program Files"/"Microsoft Visual Studio"/2022/Community/VC/Auxiliary/Build/vcvarsall.bat x64

# /clr:nostdlib	Ignore the system .NET framework directory when searching for assemblies.
# 
# /D<name>{=|#}<text>	Defines constants and macros.
# 
# /F	Sets stack size.
# 
# /Fe	Renames the executable file.
# 
# /fsanitize	Enables compilation of sanitizer instrumentation such as AddressSanitizer.
# 
# /I<dir>	Searches a directory for include files.
# 
# /O2	Creates fast code.
# 
# /Od	Disables optimization.
# 
# /Oi[-]	Generates intrinsic functions.
# 
# /options:strict	Unrecognized compiler options are errors.
# 
# 
# /std:c11	C11 standard ISO/IEC 9899:2011.
# note (Jelle): There doesn't seem to be an option for C99 T_T
# 
# /TC	Specifies all source files are C.
# 
# /Wall	Enable all warnings, including warnings that are disabled by default.
# 
# /WX	Treat warnings as errors.

COMPILER_ARGS="
-options:strict
-TC
-std:c11
-WX
-I"../sampleproject_src/"
-I"../engine_src/windows/"
-I"../engine_src/shared/debigulator/src/"
-I"../engine_src/shared/"
-I"../engine_src/shared_apple/"
-I"../engine_src/shared_linux_apple/"
-I"../engine_src/shared_windows_macos/"
-I"../engine_src/shared_opengl/"
-I"../sampleproject_src/"
"

if [[ $1 = "DEBUG" ]]; then
COMPILER_ARGS_EXTRA="-Zi -Od"
else
    if [[ $1 = "RELEASE" ]]; then
    COMPILER_ARGS_EXTRA="-O2"
    fi
fi

echo "create build folder..>"
mkdir -p build/windows/$APP_NAME
# mkdir -p build/windows/$APP_NAME

echo "deleting previous build..."
rm -r -f build/windows/$APP_NAME/*.txt
rm -r -f build/windows/$APP_NAME/$APP_NAME
# rm -r -f build/windows/$APP_NAME/$APP_NAME.dsym

############

# do stuff

############
echo "copy resources..."

pushd resources > /dev/null
for extension in png obj dat
do
    for f in *.$extension;
    do
    if test -f "../build/windows/$APP_NAME/$f"; then
        echo "$f was already in build folder...."
    else
        echo "copying resource file $f to build folder..."
        cp -r -f $f ../build/windows/$APP_NAME/$f
    fi
    done
done
popd > /dev/null
############

pushd build
cl $COMPILER_ARGS $COMPILER_ARGS_EXTRA ../unitybuild_windows.c -Fe"windows/$APP_NAME/$APP_NAME.exe" -link user32.lib gdi32.lib opengl32.lib
popd

echo "Press any key to run program, ctrl-C to abort..."
read -n 1 -s
build/windows/$APP_NAME/$APP_NAME.exe
