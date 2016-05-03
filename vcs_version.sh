#!/bin/sh

echo "#define VCS_VERSION \"`git describe --tags`\"" > vcs_version.h

