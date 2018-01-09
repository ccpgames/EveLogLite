#!/bin/sh

mkdir icon.iconset

cp 16.png icon.iconset/icon_16x16.png
cp 32.png icon.iconset/icon_16x16@2x.png
cp 32.png icon.iconset/icon_32x32.png
cp 64.png icon.iconset/icon_32x32@2x.png
cp 64.png icon.iconset/icon_64x64.png
cp 128.png icon.iconset/icon_64x64@2x.png
cp 128.png icon.iconset/icon_128x128.png
cp 256.png icon.iconset/icon_128x128@2x.png
cp 256.png icon.iconset/icon_256x256.png
cp 512.png icon.iconset/icon_256x256@2x.png
cp 512.png icon.iconset/icon_512x512.png
cp 1024.png icon.iconset/icon_512x512@2x.png

iconutil -c ../../icon.icns icon.iconset
rm -R icon.iconset