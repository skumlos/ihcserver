#!/bin/sh

if [ "$#" -lt "1" ]; then
	echo "Usage $0 <version>";
	exit 1;
fi

echo "Creating ihcserver-$1.tar.gz";

tar czf ihcserver-$1.tar.gz * --exclude='ihcserver' --exclude='*git*' --exclude='*.o' --exclude='*.a' --exclude=$(basename $0) --exclude='*.gz'
