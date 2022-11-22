#!/bin/sh
#
# Copyright (C) 2021, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

# Exit immediately if a command exits with a non-zero status.
set -e

MPIRUN=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$1/g"`

err=0
for p in ${check_PROGRAMS} ; do
   for vol_type in "terminal" "passthru"; do
      if test "x${vol_type}" = xterminal ; then
         outfile="${TESTOUTDIR}/${p}.h5"
         unset H5VL_LOG_PASSTHRU_READ_WRITE
      else
         outfile="${TESTOUTDIR}/${p}-passthru.h5"
         export H5VL_LOG_PASSTHRU_READ_WRITE=1
      fi

      ${MPIRUN} ./${p} ${outfile}

      unset H5VL_LOG_PASSTHRU_READ_WRITE
   done
done
exit $err