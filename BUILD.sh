#!/bin/sh

# This shell script builds everyting on an TravisCI Linux (Ubunutu) worker
set -ev
export UNATTENDED="--assume-yes"
export MAKE_PARALLEL="-j $(nproc)"
export PDF2HTMLEX_BRANCH=`git describe --always`
export PDF2HTMLEX_PREFIX=/usr/local
export PDF2HTMLEX_PATH=/usr/local/bin/pdf2htmlEX
export DEBIAN_FRONTEND=noninteractive

################
# do the build

./buildScripts/versionEnvs
./buildScripts/reportEnvs
./buildScripts/getPoppler
./buildScripts/getZipper
./buildScripts/buildPoppler
./buildScripts/getFontforge
./buildScripts/buildFontforge
./buildScripts/buildZipper
./buildScripts/buildPdf2htmlEX
