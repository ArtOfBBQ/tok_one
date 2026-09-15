APP_NAME="loreseek"

COMPILER_PATHS="
-I engine_src/macos/
-I engine_src/shared/debigulator/src/
-I engine_src/shared/
-I engine_src/shared_apple/
-I engine_src/shared_linux_apple/
-I engine_src/shared_windows_macos/
-I sampleproject_src/
"

COMPILER_ARGS="
-ferror-limit=2 
-march=native 
-Wall 
-x objective-c -std=c11 
-O0 
-include T1_macro_settings.h
-c"

if [[ $1 = "DEBUG" ]]; then
COMPILER_ARGS_EXTRA="-g"
else
    if [[ $1 = "RELEASE" ]]; then
    COMPILER_ARGS_EXTRA=""
    fi
fi

MAC_FRAMEWORKS="
    -framework AppKit 
    -framework MetalKit 
    -framework Metal
    -framework AudioToolbox"

echo "create build folder..>"
sudo mkdir -p build/lib/macos

echo "deleting previous build..."
sudo rm -r -f build/lib/macos/*.*

SOURCE_FILES="
engine_src/shared/debigulator/src/inflate.c
engine_src/shared/debigulator/src/decode_png.c
engine_src/shared/debigulator/src/decode_bmp.c
engine_src/shared/T1_settings.c
engine_src/shared/T1_texquad.c
engine_src/shared/T1_linalg3d.c
engine_src/shared/T1_easing.c
engine_src/shared/T1_meta.c
engine_src/shared/T1_img.c
engine_src/shared/T1_wav.c
engine_src/shared/T1_token.c
engine_src/shared/T1_objparser.c
engine_src/shared/T1_mtlparser.c
engine_src/shared/T1_std.c
engine_src/shared/T1_log.c
engine_src/shared/T1_mesh_summary.c
engine_src/shared/T1_collision.c
engine_src/shared/T1_id.c
engine_src/shared/T1_audio.c
engine_src/shared/T1_global.c
engine_src/shared/T1_triangle.c
engine_src/shared/T1_material.c
engine_src/shared/T1_zlight.c
engine_src/shared/T1_types_cpu_to_gpu.c
engine_src/shared_apple/T1_apple_audio.m
engine_src/shared/T1_platform_layer_common.c
engine_src/macos/T1_macos_platform_layer.m
engine_src/shared_apple/T1_apple_platform_layer.m
engine_src/shared_linux_apple/T1_linux_apple_platform_layer.c
engine_src/shared/T1_mem.c
engine_src/shared/T1_profiler.c
engine_src/shared/T1_objmodel.c
engine_src/shared/T1_io.c
engine_src/shared/T1_rand.c
engine_src/shared_apple/T1_gpu.m
engine_src/shared/T1_tex.c
engine_src/shared/T1_tex_array.c
engine_src/shared/T1_tex_files.c
engine_src/shared/T1_zsprite.c
engine_src/shared/T1_particle.c
engine_src/shared/T1_anim.c
engine_src/shared/T1_text.c
engine_src/shared/T1_render_view.c
engine_src/shared/T1_ui_widget.c
engine_src/shared/T1_term.c
engine_src/shared/T1_render.c
engine_src/shared/T1_gameloop.c
engine_src/shared/T1_appinit.c
engine_src/shared/T1.c"

echo "Compiling & linking $APP_NAME..."
if
sudo time gcc $COMPILER_PATHS $COMPILER_ARGS $COMPILER_ARGS_EXTRA $MAC_FRAMEWORKS $SOURCE_FILES
then
echo "compilation succesful, archiving..."
mv *.o build/lib/macos
ar rcs build/lib/macos/T1.a build/lib/macos/*.o
rm build/lib/macos/*.o
cp engine_src/shared/T1.h build/lib/macos/T1.h
cp engine_src/shared/T1_types_public.h build/lib/macos/T1_types_public.h
cp engine_src/shared/T1_types_public_gpucpu.h build/lib/macos/T1_types_public_gpucpu.h
cp engine_src/shared/T1_macro_settings.h build/lib/macos/T1_macro_settings.h

else
echo "compilation failed"
exit 0
fi

