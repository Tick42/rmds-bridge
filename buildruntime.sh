
##
## copy all the runtime components to a location
# $1 destination

#open mama runtime - cpp and c libs
cp $OPENMAMA_API/lib/libmamacpp.s* $1
cp $OPENMAMA_API/lib/libmama.s* $1

# mamalistenc sample
#cp $OPENMAMA_API/bin/mamalistenc* $1

# get the versions of the test apps that we have built
cp $TICK42_BRIDGE/build/mamaClient/mamalistenc $1
cp $TICK42_BRIDGE/build/mamaClient/mamalistencpp $1

# bridge binaries
cp $TICK42_BRIDGE/build/lib/* $1

# TR UPA binaries
cp $TICK42_UPA//Libs/RHEL6_64_GCC444/Optimized/Shared/librssl* $1

# TR Data dictionary
cp $TICK42_UPA/etc/* $1

# mamalistenc shell script
cp $TICK42_BRIDGE/mamaClient/mamalistenc.sh $1

# sample mama.properties 
cp $TICK42_BRIDGE/tick42rmds/mama.properties $1

# sample mama dictionary
cp $TICK42_BRIDGE/tick42rmds/mama_dict.txt $1

# sample field map
cp $TICK42_BRIDGE/tick42rmds/fieldmap.csv $1

# use LD_LIBRARY_PATH for boost
export LD_LIBRARY_PATH=$TICK42_BOOST/stage/lib