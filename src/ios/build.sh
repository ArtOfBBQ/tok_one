echo "building for iOS on connected device..."
echo "WARNING: BUILDING FOR IOS TAKES EONS, TYPE CTRL-C to CANCEL"

echo "deleting previous build..."
rm -r build/ios
mkdir build/ios

echo "creating app archive..."
xcodebuild -project src/ios/hello3dgfx/hello3dgfx.xcodeproj archive -scheme hello3dgfx -sdk iphoneos -allowProvisioningUpdates -archivePath build/ios/hello3dgfx.xcarchive

echo "creating .ipa file from app archive..."
xcodebuild -exportArchive -archivePath build/ios/hello3dgfx.xcarchive -exportPath build/ios -exportOptionsPlist src/ios/hello3dgfx/hello3dgfx/info.plist

echo "installing .ipa file on device..."
fastlane run install_on_device device_id:"e86c1c6cbc8630cee4d1f80d84222f52c38cf17a" ipa:"build/ios/hello3dgfx.ipa"

