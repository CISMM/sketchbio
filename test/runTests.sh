#!/bin/sh

[ -z "${SSH_AUTH_SOCK}" ] && eval $(ssh-agent)

ssh-add /path/to/private/key 2>&1 > /dev/null

cd /path/to/nightly/build/dir

ctest -S sketchbio_nightly.cmake -V 2>&1 > /playpen/sketchbio/nightly_builds/lastnight.log


