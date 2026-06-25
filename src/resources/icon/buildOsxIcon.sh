#!/bin/sh

BINARY_DIR=$1
ICONSET_DIR=$BINARY_DIR/icon.iconset
mkdir -p $ICONSET_DIR
SCRIPT_PATH=$(dirname $0)

cp $SCRIPT_PATH/16.png $ICONSET_DIR/icon_16x16.png
cp $SCRIPT_PATH/32.png $ICONSET_DIR/icon_16x16@2x.png
cp $SCRIPT_PATH/32.png $ICONSET_DIR/icon_32x32.png
cp $SCRIPT_PATH/64.png $ICONSET_DIR/icon_32x32@2x.png
cp $SCRIPT_PATH/64.png $ICONSET_DIR/icon_64x64.png
cp $SCRIPT_PATH/128.png $ICONSET_DIR/icon_64x64@2x.png
cp $SCRIPT_PATH/128.png $ICONSET_DIR/icon_128x128.png
cp $SCRIPT_PATH/256.png $ICONSET_DIR/icon_128x128@2x.png
cp $SCRIPT_PATH/256.png $ICONSET_DIR/icon_256x256.png
cp $SCRIPT_PATH/512.png $ICONSET_DIR/icon_256x256@2x.png
cp $SCRIPT_PATH/512.png $ICONSET_DIR/icon_512x512.png
cp $SCRIPT_PATH/1024.png $ICONSET_DIR/icon_512x512@2x.png

iconutil -c icns -o $BINARY_DIR/icon.icns $ICONSET_DIR
rm -r $ICONSET_DIR
