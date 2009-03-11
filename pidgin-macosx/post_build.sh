#!/bin/bash

cd "$SRCROOT/../pidgin-src"
echo $TARGET_BUILD_DIR/$CONTENTS_FOLDER_PATH
RSC="$TARGET_BUILD_DIR/$CONTENTS_FOLDER_PATH/Resources"

mkdir -p "$RSC"
cp "$SRCROOT/pidgin.icns" "$RSC/"

mkdir -p "$RSC/data"
mkdir -p "$RSC/data/pixmaps/pidgin"
rsync -av --delete pidgin/pixmaps/* "$RSC/data/pixmaps/pidgin/"

mkdir -p "$RSC/data/pixmaps/pidgin/protocols/16"
cp ../pidgin-facebook*/facebook16.png "$RSC/data/pixmaps/pidgin/protocols/16/facebook.png"
mkdir -p "$RSC/data/pixmaps/pidgin/protocols/22"
cp ../pidgin-facebook*/facebook22.png "$RSC/data/pixmaps/pidgin/protocols/22/facebook.png"
mkdir -p "$RSC/data/pixmaps/pidgin/protocols/48"
cp ../pidgin-facebook*/facebook48.png "$RSC/data/pixmaps/pidgin/protocols/48/facebook.png"

rsync -a ../gfire*/pixmaps/{16,22,48} "$RSC/data/pixmaps/pidgin/protocols"

mkdir -p "$RSC/data/sounds/purple"
rsync -av --delete share/sounds/* "$RSC/data/sounds/purple/"
mkdir -p "$RSC/data/purple"
rsync -av --delete share/ca-certs "$RSC/data/purple/"

mkdir -p "$RSC/conf"
cp "$SRCROOT/gtkrc" "$RSC/conf/"

mkdir -p "$RSC/qq"
mkdir -p "$RSC/sysconf"
mkdir -p "$RSC/locale"
mkdir -p "$RSC/lib"

sh "$SRCROOT/fix_binary.sh" "$TARGET_BUILD_DIR/$WRAPPER_NAME"

exit

# use this if you want to use the system frameworks
for f in Cairo GLib Gtk; do
	install_name_tool -change \
	"@executable_path/../Frameworks/$f.framework/$f" \
	"/Library/Frameworks/$f.framework/$f" \
	"$TARGET_BUILD_DIR/$EXECUTABLE_PATH"
done
