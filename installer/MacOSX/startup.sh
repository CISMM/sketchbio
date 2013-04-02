#!/bin/sh
#
# Author: Cory Quammen
# Note: This file will be renamed to MicroscopeSimulator in MicroscopeSimulator.app/Contents/MacOS/
#
# Copied and modified for SketchBio by: Shawn Waldon
#

echo "Running SketchBio executable."

BUNDLE_PATH="`echo "$0" | sed -e 's/\/Contents\/MacOS\/SketchBio\ [0-9]*.[0-9]*.[0-9]*//'`"
EXECUTABLE_PATH="$BUNDLE_PATH/Contents/Resources/bin/SketchBio"

export "DYLD_LIBRARY_PATH=$BUNDLE_PATH/Contents/Resources/lib/"
export "DYLD_FRAMEWORK_PATH=$BUNDLE_PATH/Contents/Frameworks/"

exec "$EXECUTABLE_PATH"

