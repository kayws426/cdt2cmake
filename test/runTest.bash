#!/bin/bash

#given that logic is a crime against bash I will resort to copypasta
#CDT2CMAKE=$(pwd)/src/cdt2cmake
#
#function try() {
#         "$@" || (e=$?; echo "$@" > /dev/stderr; exit $e)
#}
#
#function do_test() {
#       TEST_DIR=$1
#       pushd $TEST_DIR
#
#       if [[ $($CDT2CMAKE . | wc -l ) -gt 1 ]] ; then
#               $CDT2CMAKE . > CMakeLists.txt
#               cmake . 1>&2
#               make all 1>&2
#
#               #CLEANUP
#               make clean
#               find -iname "*cmake*" -type d -exec rm -rf {} \; 2>/dev/null
#               find -iname "*cmake*" -exec rm {} \;
#               find -iname "Makefile" -exec rm {} \;
#               popd &>/dev/null
#               exit 0
#       else
#               popd &>/dev/null
#               exit 1
#       fi
#}
#
#for hit in $(find test -maxdepth 2 -mindepth 2 -type d); do
#       echo -e "TESTING: $hit";
#       if $(do_test $hit); then
#               echo "$(basename $hit): SUCCESS"
#       else
#               echo "$(basename $hit): FAILURE"
#       fi
#done

retVal=0
CDT2CMAKE="$(pwd)/../src/cdt2cmake"

pushd projects/helloc &>/dev/null
if [[ $($CDT2CMAKE . | wc -l ) -gt 1 ]] ; then
        $CDT2CMAKE . > CMakeLists.txt
        cmake . 1>&2 || (echo "helloc: FAILURE"; retVal=$retVal+1)
        make all 1>&2 || (echo "helloc: FAILURE"; retVal=$retVal+1)

        echo "helloc: SUCCESS"
        make clean
        find -iname "*cmake*" -type d -exec rm -rf {} \; 2>/dev/null
        find -iname "*cmake*" -exec rm {} \;
        find -iname "Makefile" -exec rm {} \;
else
        echo "helloc: FAILURE"; retVal=$retVal+1
fi
popd &>/dev/null

pushd projects/helloc++ &>/dev/null
if [[ $($CDT2CMAKE . | wc -l ) -gt 1 ]] ; then
        $CDT2CMAKE . > CMakeLists.txt
        cmake . 1>&2 || (echo "helloc: FAILURE"; retVal=$retVal+1)
        make all 1>&2 || (echo "helloc: FAILURE"; retVal=$retVal+1)

        echo "helloc++: SUCCESS"
        make clean
        find -iname "*cmake*" -type d -exec rm -rf {} \; 2>/dev/null
        find -iname "*cmake*" -exec rm {} \;
        find -iname "Makefile" -exec rm {} \;
else
        echo "helloc: FAILURE"; retVal=$retVal+1
fi
popd &>/dev/null

pushd projects/simplec++exe &>/dev/null
if [[ $($CDT2CMAKE . | wc -l ) -gt 1 ]] ; then
        $CDT2CMAKE . > CMakeLists.txt
        cmake . 1>&2 || (echo "helloc: FAILURE"; retVal=$retVal+1)
        make all 1>&2 || (echo "helloc: FAILURE"; retVal=$retVal+1)

        echo "helloc++exe: SUCCESS"
        make clean
        find -iname "*cmake*" -type d -exec rm -rf {} \; 2>/dev/null
        find -iname "*cmake*" -exec rm {} \;
        find -iname "Makefile" -exec rm {} \;
else
        echo "helloc: FAILURE"; retVal=$retVal+1
fi
popd &>/dev/null

pushd projects/simplec++staticlib &>/dev/null
if [[ $($CDT2CMAKE . | wc -l ) -gt 1 ]] ; then
        $CDT2CMAKE . > CMakeLists.txt
        cmake . 1>&2 || (echo "helloc: FAILURE"; retVal=$retVal+1)
        make all 1>&2 || (echo "helloc: FAILURE"; retVal=$retVal+1)

        echo "simplec++staticlib: SUCCESS"
        make clean
        find -iname "*cmake*" -type d -exec rm -rf {} \; 2>/dev/null
        find -iname "*cmake*" -exec rm {} \;
        find -iname "Makefile" -exec rm {} \;
else
        echo "helloc: FAILURE"; retVal=$retVal+1
fi
popd &>/dev/null

pushd projects/simplec++sharedlib &>/dev/null
if [[ $($CDT2CMAKE . | wc -l ) -gt 1 ]] ; then
        $CDT2CMAKE . > CMakeLists.txt
        cmake . 1>&2 || (echo "helloc: FAILURE"; retVal=$retVal+1)
        make all 1>&2 || (echo "helloc: FAILURE"; retVal=$retVal+1)

        echo "simplec++sharedlib: SUCCESS"
        make clean
        find -iname "*cmake*" -type d -exec rm -rf {} \; 2>/dev/null
        find -iname "*cmake*" -exec rm {} \;
        find -iname "Makefile" -exec rm {} \;
else
        echo "helloc: FAILURE"; retVal=$retVal+1
fi
popd &>/dev/null

exit $retVal
