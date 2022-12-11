echo "Deleting buggy, useless, resource intensive and in all other ways awful Apple malware..."
rm -r /Users/jellevandeneynde/Library/Developer/Xcode/DerivedData

# bash src/linux/build.sh
bash src/macos/build.sh
# bash src/macos/buildtests.sh
# bash src/ios/build.sh

