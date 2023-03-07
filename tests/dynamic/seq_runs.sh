#!/bin/bash -l
#
# Copyright (C) 2022, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

RUN_CMD=${TESTSEQRUN}

. $srcdir/../common/wrap_runs.sh

# Some output files may be HDF5 regular files
log_vol_file_only=0

test_func $1

# test subfiling feature by setting env variable H5VL_LOG_NSUBFILES
export H5VL_LOG_NSUBFILES=-1
test_func $1

