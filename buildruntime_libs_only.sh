
##
## copy all the runtime components to a location
# $1 destination

# target directory
target=$1

# build or debug or release
mode="debug"
if [ "$2" != "" ]; then
	mode=$2
fi

cp $TICK42_BRIDGE/$mode/lib/* $target

exit 0

