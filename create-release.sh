#!/bin/sh

if [ "$#" -lt "1" ]; then
	echo "Usage $0 <version>";
	exit 1;
fi

echo "Creating ihcserver-$1.tar.gz";

tar -czf ihcserver-$1.tar.gz *.cpp *.h README.TXT utils/*.h utils/*.cpp 3rdparty/ --exclude='*git*'
