#!/bin/sh
#
# Copyright (C) 2021, Northwestern University and Argonne National Laboratory
# See COPYRIGHT notice in top-level directory.
#

MPIRUN=`echo ${TESTMPIRUN} | ${SED} -e "s/NP/$1/g"`

if test "x${HDF5_VOL_CONNECTOR}" != x ; then
   async_vol=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "under_vol=512"`
   if test "x$?" = x0 ; then
      async_vol=yes
   fi
   cache_vol=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "under_vol=513"`
   if test "x$?" = x0 ; then
      cache_vol=yes
   fi
   log_vol=`echo "${HDF5_VOL_CONNECTOR}" | ${EGREP} -- "LOG "`
   if test "x$?" = x0 ; then
      log_vol=yes
   fi
fi

# Exit immediately if a command exits with a non-zero status.
set -e

err=0
for p in ${check_PROGRAMS} ; do
   for vol_type in "terminal" "passthru"; do
      if test "x${vol_type}" = xterminal ; then
         if test "x${cache_vol}" = xyes || test "x${async_vol}" = xyes ; then
            continue
         fi
         outfile="${TESTOUTDIR}/${p}.h5"
         unset H5VL_LOG_PASSTHRU
      else
         outfile="${TESTOUTDIR}/${p}-passthru.h5"
         export H5VL_LOG_PASSTHRU=1
      fi

      ${MPIRUN} ./${p} ${outfile}

      unset H5VL_LOG_PASSTHRU

      if test "x${log_vol}" != xyes ; then
         continue
      fi

      FILE_KIND=`${top_builddir}/utils/h5ldump/h5ldump -k $outfile`
      if test "x${FILE_KIND}" != xHDF5-LogVOL ; then
         echo "Error (as $vol_type VOL): Output file $outfile is not Log VOL, but ${FILE_KIND}"
         err=1
      fi
   done
done
exit $err

