/*
 *  Copyright (C) 2022, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fcntl.h>
#include <mpi.h>
#include <sys/stat.h>
#include <sys/types.h>
//
#include "H5VL_log.h"
#include "H5VL_log_file.hpp"
#include "H5VL_log_filei.hpp"
#include "H5VL_log_info.hpp"
#include "H5VL_logi.hpp"
#include "H5VL_logi_util.hpp"

/********************* */
/* Function prototypes */
/********************* */
const H5VL_file_class_t H5VL_log_file_g {
    H5VL_log_file_create,   /* create */
    H5VL_log_file_open,     /* open */
    H5VL_log_file_get,      /* get */
    H5VL_log_file_specific, /* specific */
    H5VL_log_file_optional, /* optional */
    H5VL_log_file_close     /* close */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_create
 *
 * Purpose:     Creates a container using this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_file_create (
    const char *name, unsigned flags, hid_t fcpl_id, hid_t fapl_id, hid_t dxpl_id, void **req) {
    herr_t err = 0;
    int mpierr;
    H5VL_log_info_t *info = NULL;
    H5VL_log_file_t *fp   = NULL;
    H5VL_loc_params_t loc;
    hid_t uvlid;
    hid_t fdid;  // VFL driver ID
    hid_t ufcplid = H5I_INVALID_HID;
    void *under_vol_info;
    MPI_Comm comm    = MPI_COMM_SELF;
    MPI_Info mpiinfo = MPI_INFO_NULL;
    int attbuf[5];
    H5VL_logi_err_finally finally ([&ufcplid] () -> void {
        if (ufcplid != H5I_INVALID_HID) H5Pclose (ufcplid);
    });

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) {
            printf ("H5VL_log_file_create(%s, %u, fcpl_id, fapl_id, dxpl_id, %p)\n", name, flags,
                    req);
        }
#endif

        fp = H5VL_log_filei_search (name);
        if (fp) {
            fp = NULL;
            RET_ERR ("File already exist")
        }

        H5VL_LOGI_PROFILING_TIMER_START;
        // Try get info about under VOL
        H5Pget_vol_info (fapl_id, (void **)&info);

        if (info) {
            uvlid          = info->uvlid;
            under_vol_info = info->under_vol_info;
        } else {  // If no under VOL specified, use the native VOL
            htri_t ret;
            ret = H5VLis_connector_registered_by_name ("native");
            if (ret != 1) { ERR_OUT ("Native VOL not found") }
            uvlid = H5VLpeek_connector_id_by_name ("native");
            CHECK_ID (uvlid)
            under_vol_info = NULL;
        }

        // Make sure we have mpi enabled
        fdid = H5Pget_driver (fapl_id);
        CHECK_ID (fdid)
        if (fdid == H5FD_MPIO) {
            err = H5Pget_fapl_mpio (fapl_id, &comm, &mpiinfo);
        } else
            err = -1;
        if (err != 0) {  // No MPI, use MPI_COMM_WORLD
            comm    = MPI_COMM_SELF;
            mpiinfo = MPI_INFO_NULL;
        }

        // Init file obj
        fp         = new H5VL_log_file_t (uvlid);
        fp->flag   = flags;
        fp->nldset = 0;
        fp->nmdset = 0;
        fp->ndset  = 0;
        fp->config = 0;
        fp->mdsize = 0;
        fp->zbsize = 0;
        fp->zbuf   = NULL;
        fp->is_log_based_file = true;
        mpierr     = MPI_Comm_dup (comm, &(fp->comm));
        CHECK_MPIERR
        if (mpiinfo != MPI_INFO_NULL) {
            mpierr = MPI_Info_dup (mpiinfo, &(fp->info));
            CHECK_MPIERR
        } else {
            fp->info = MPI_INFO_NULL;
        }
        mpierr = MPI_Comm_rank (comm, &(fp->rank));
        CHECK_MPIERR
        mpierr = MPI_Comm_size (comm, &(fp->np));
        CHECK_MPIERR
        fp->dxplid = H5Pcopy (dxpl_id);
        fp->name   = std::string (name);
        err        = H5Pget_nb_buffer_size (fapl_id, &(fp->bsize));
        CHECK_ERR
        H5VL_log_filei_parse_fapl (fp, fapl_id);

        H5VL_log_filei_parse_fcpl (fp, fcpl_id);

        H5VL_log_filei_init_idx (fp);

        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_CREATE_INIT);

        // Create the file with underlying VOL
        H5VL_LOGI_PROFILING_TIMER_START;
        fp->ufaplid = H5VL_log_filei_get_under_plist (fapl_id);
        err         = H5Pset_vol (fp->ufaplid, uvlid, under_vol_info);
        CHECK_ERR
        err = H5Pset_all_coll_metadata_ops (fp->ufaplid, (hbool_t) false);
        CHECK_ERR
        err = H5Pset_coll_metadata_write (fp->ufaplid, (hbool_t) true);
        CHECK_ERR
        // err = H5Pset_alignment (fp->ufaplid, 4096, 4096);
        // CHECK_ERR
        ufcplid = H5VL_log_filei_get_under_plist(fcpl_id);
        CHECK_ID(ufcplid)

        H5VL_LOGI_PROFILING_TIMER_START;
        fp->uo = H5VLfile_create (name, flags, ufcplid, fp->ufaplid, dxpl_id, NULL);
        CHECK_PTR (fp->uo)
        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLFILE_CREATE);
        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_CREATE_FILE);

        // Figure out lustre configuration
        H5VL_LOGI_PROFILING_TIMER_START;
        if (fp->config & H5VL_FILEI_CONFIG_DATA_ALIGN) {
            H5VL_log_filei_parse_strip_info (fp);
            // Dummy stripe setting for debugging without lustre
            // fp->scount=2;
            // fp->ssize=8388608;
            if ((fp->scount <= 0) || (fp->ssize <= 0)) {
                fp->config &= ~H5VL_FILEI_CONFIG_DATA_ALIGN;
                if (fp->rank == 0) {
                    printf (
                        "Warning: Cannot retrive stripping info, disable aligned data layout\n");
                }
            }
        }
        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_CREATE_STRIPE);

        H5VL_LOGI_PROFILING_TIMER_START;
        if ((fp->config & H5VL_FILEI_CONFIG_DATA_ALIGN) ||
            (fp->config & H5VL_FILEI_CONFIG_SUBFILING)) {
            H5VL_log_filei_calc_node_rank (fp);
        } else {
            fp->group_rank = fp->rank;
            fp->group_np   = fp->np;
            fp->group_comm = fp->comm;
            fp->group_id   = 0;
            fp->ngroup     = 1;
        }
        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_CREATE_GROUP_RANK);

        H5VL_LOGI_PROFILING_TIMER_START;
        if (fp->config & H5VL_FILEI_CONFIG_SUBFILING) {
            // Aligned write not supported in subfiles
            fp->config &= ~H5VL_FILEI_CONFIG_DATA_ALIGN;

            H5VL_log_filei_create_subfile (fp, flags, fp->ufaplid, dxpl_id);
        } else {
            fp->sfp     = fp->uo;
            fp->subname = fp->name;
        }
        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_CREATE_SUBFILE);

        // Create the LOG group
        H5VL_LOGI_PROFILING_TIMER_START;
        loc.obj_type = H5I_FILE;
        loc.type     = H5VL_OBJECT_BY_SELF;
        fp->lgp      = H5VLgroup_create (fp->sfp, &loc, fp->uvlid, H5VL_LOG_FILEI_GROUP_LOG,
                                    H5P_LINK_CREATE_DEFAULT, H5P_GROUP_CREATE_DEFAULT,
                                    H5P_GROUP_CREATE_DEFAULT, dxpl_id, NULL);
        CHECK_PTR (fp->lgp)
        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_CREATE_GROUP);

        if (fp->config & H5VL_FILEI_CONFIG_DATA_ALIGN) {
            fp->fd = open (name, O_RDWR);
            if (fp->fd < 0) { ERR_OUT ("open fail") }
        } else {
            fp->fd = -1;
        }

        // Open the file with MPI
        H5VL_LOGI_PROFILING_TIMER_START;
        mpierr =
            MPI_File_open (fp->group_comm, fp->subname.c_str (), MPI_MODE_RDWR, mpiinfo, &(fp->fh));
        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_CREATE_FH);
        CHECK_MPIERR

        // Att
        attbuf[0] = fp->ndset;
        attbuf[1] = fp->nldset;
        attbuf[2] = fp->nmdset;
        attbuf[3] = fp->config;
        attbuf[4] = fp->ngroup;
        H5VL_logi_add_att (fp, H5VL_LOG_FILEI_ATTR_INT, H5T_STD_I32LE, H5T_NATIVE_INT32, 5, attbuf,
                           dxpl_id, NULL);

        H5VL_log_filei_register (fp);

        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_CREATE);
    }
    H5VL_LOGI_EXP_CATCH

    goto fn_exit;
err_out:;
    if (fp) { delete fp; }
    fp = NULL;
fn_exit:;
    if (comm != MPI_COMM_SELF) { MPI_Comm_free (&comm); }
    if (mpiinfo != MPI_INFO_NULL) { MPI_Info_free (&mpiinfo); }
    if (info) { free (info); }

    return (void *)fp;
} /* end H5VL_log_file_create() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_open
 *
 * Purpose:     Opens a container created with this connector
 *
 * Return:      Success:    Pointer to a file object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_file_open (
    const char *name, unsigned flags, hid_t fapl_id, hid_t dxpl_id, void **req) {
    herr_t err = 0;
    int mpierr;
    H5VL_log_info_t *info = NULL;
    H5VL_log_file_t *fp   = NULL;
    hid_t uvlid;
    hid_t fdid;  // VFL driver ID
    void *under_vol_info;
    MPI_Comm comm    = MPI_COMM_SELF;
    MPI_Info mpiinfo = MPI_INFO_NULL;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) {
            printf ("H5VL_log_file_open(%s, %u, fapl_id, dxpl_id, %p)\n", name, flags, req);
        }
#endif

        fp = H5VL_log_filei_search (name);
        if (fp) {
            fp = NULL;
            RET_ERR ("File already exist")
        }

        // Try get info about under VOL
        H5Pget_vol_info (fapl_id, (void **)&info);

        if (info) {
            uvlid          = info->uvlid;
            under_vol_info = info->under_vol_info;
        } else {  // If no under VOL specified, use the native VOL
            htri_t ret;
            ret = H5VLis_connector_registered_by_name ("native");
            if (ret != 1) { ERR_OUT ("Native VOL not found") }
            uvlid = H5VLpeek_connector_id_by_name ("native");
            CHECK_ID (uvlid)
            under_vol_info = NULL;
            // return NULL;
        }

        // Make sure we have mpi enabled
        fdid = H5Pget_driver (fapl_id);
        CHECK_ID (fdid)
        if (fdid == H5FD_MPIO) {
            err = H5Pget_fapl_mpio (fapl_id, &comm, &mpiinfo);
        } else
            err = -1;
        if (err != 0) {  // No MPI, use MPI_COMM_WORLD
            comm    = MPI_COMM_SELF;
            mpiinfo = MPI_INFO_NULL;
        }

        // Init file obj
        fp         = new H5VL_log_file_t (uvlid);
        fp->flag   = flags;
        fp->config = 0;
        fp->fd     = -1;
        fp->sfp    = NULL;
        fp->lgp    = NULL;
        fp->mdsize = 0;
        fp->zbsize = 0;
        fp->zbuf   = NULL;
        fp->is_log_based_file = true;
        mpierr     = MPI_Comm_dup (comm, &(fp->comm));
        CHECK_MPIERR
        if (mpiinfo != MPI_INFO_NULL) {
            mpierr = MPI_Info_dup (mpiinfo, &(fp->info));
            CHECK_MPIERR
        } else {
            fp->info = MPI_INFO_NULL;
        }
        mpierr = MPI_Comm_rank (comm, &(fp->rank));
        CHECK_MPIERR
        mpierr = MPI_Comm_size (comm, &(fp->np));
        CHECK_MPIERR
        fp->dxplid = H5Pcopy (dxpl_id);
        fp->name   = std::string (name);
        err        = H5Pget_nb_buffer_size (fapl_id, &(fp->bsize));
        CHECK_ERR

        // Create the file with underlying VOL
        fp->ufaplid = H5VL_log_filei_get_under_plist (fapl_id);
        err         = H5Pset_vol (fp->ufaplid, uvlid, under_vol_info);
        CHECK_ERR
        err = H5Pset_all_coll_metadata_ops (fp->ufaplid, (hbool_t) false);
        CHECK_ERR
        err = H5Pset_coll_metadata_write (fp->ufaplid, (hbool_t) true);
        CHECK_ERR
        // err = H5Pset_alignment (fp->ufaplid, 4096, 4096);
        // CHECK_ERR
        H5VL_LOGI_PROFILING_TIMER_START;
        fp->uo = H5VLfile_open (name, flags, fp->ufaplid, dxpl_id, NULL);
        CHECK_PTR (fp->uo)
        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLFILE_OPEN);

        // Fapl property can overwrite config in file, parse after loading config
        H5VL_log_filei_parse_fapl (fp, fapl_id);

        H5VL_log_filei_register (fp);

        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_OPEN);
    }
    H5VL_LOGI_EXP_CATCH

    goto fn_exit;
err_out:;
    if (fp) { delete fp; }
    fp = NULL;
fn_exit:;
    if (comm != MPI_COMM_SELF) { MPI_Comm_free (&comm); }
    if (mpiinfo != MPI_INFO_NULL) { MPI_Info_free (&mpiinfo); }
    if (info) { free (info); }
    return (void *)fp;
} /* end H5VL_log_file_open() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_get
 *
 * Purpose:     Get info about a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_get (void *file, H5VL_file_get_args_t *args, hid_t dxpl_id, void **req) {
    herr_t err         = 0;
    H5VL_log_obj_t *op = (H5VL_log_obj_t *)file;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) {
            printf ("H5VL_log_file_get(%p, args, dxpl_id, %p, ...)\n", file, req);
        }
#endif

        H5VL_LOGI_PROFILING_TIMER_START;
        err = H5VLfile_get (op->uo, op->uvlid, args, dxpl_id, req);
        CHECK_ERR
        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VLFILE_GET);

        H5VL_LOGI_PROFILING_TIMER_STOP (op->fp, TIMER_H5VL_LOG_FILE_GET);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
} /* end H5VL_log_file_get() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_specific
 *
 * Purpose:     Specific operation on file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_specific (void *file,
                               H5VL_file_specific_args_t *args,
                               hid_t dxpl_id,
                               void **req) {
    herr_t err          = 0;
    H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

    try {
#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) {
            printf ("H5VL_log_file_specific(%p, args, dxplid, %p, ...)\n", file, req);
        }
#endif


        switch (args->op_type) {
            case H5VL_FILE_REOPEN: {
                *(args->args.reopen.file) = file;
                H5VL_log_filei_inc_ref (fp);
            } break;
            case H5VL_FILE_IS_ACCESSIBLE:
            case H5VL_FILE_DELETE: {
                
                hid_t uvlid, under_fapl_id, fapl_id;
                void *under_vol_info;
                H5VL_log_info_t *info = NULL;

                // Try get info about under VOL;
                // comment: fapl_id is still correct for the case of H5VL_FILE_IS_ACCESSIBLE.
                //          this is a property(?) of union.
                fapl_id = args->args.del.fapl_id;
                
                H5Pget_vol_info (fapl_id, (void **)&info);
                if (info) {
                    uvlid          = info->uvlid;
                    under_vol_info = info->under_vol_info;
                    free (info);
                } else {  // If no under VOL specified, use the native VOL
                    htri_t ret;
                    ret = H5VLis_connector_registered_by_name ("native");
                    if (ret != 1) { ERR_OUT ("Native VOL not found") }
                    uvlid = H5VLpeek_connector_id_by_name ("native");
                    CHECK_ID (uvlid)
                    under_vol_info = NULL;
                }
                /* Call specific of under VOL */
                under_fapl_id = H5Pcopy (fapl_id);
                H5Pset_vol (under_fapl_id, uvlid, under_vol_info);
                err = H5VLfile_specific (NULL, uvlid, args, dxpl_id, req);
                CHECK_ERR
                H5Pclose (under_fapl_id);
            } break;
            case H5VL_FILE_FLUSH: {
                H5VL_LOGI_PROFILING_TIMER_START;
                if (fp->is_log_based_file) {
                    H5VL_log_nb_flush_write_reqs (fp, dxpl_id);
                } else {
                    err = H5VLfile_specific(fp->uo, fp->uvlid, args, dxpl_id, req);
                }
                H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_SPECIFIC);
            } break;
            default: {
                if (fp->is_log_based_file) {
                    ERR_OUT ("Unsupported args->op_type")
                } else {
                    err = H5VLfile_specific(fp->uo, fp->uvlid, args, dxpl_id, req);
                }
            }
                
        } /* end select */
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
} /* end H5VL_log_file_specific() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_optional
 *
 * Purpose:     Perform a connector-specific operation on a file
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_optional (void *file, H5VL_optional_args_t *args, hid_t dxpl_id, void **req) {
    herr_t err          = 0;
    H5VL_log_file_t *fp = (H5VL_log_file_t *)file;

    try {
        H5VL_LOGI_PROFILING_TIMER_START;

#ifdef LOGVOL_DEBUG
        if (H5VL_logi_debug_verbose ()) {
            printf ("H5VL_log_file_optional(%p, args, dxpl_id, %p, ...)\n", file, req);
        }
#endif

        H5VL_LOGI_PROFILING_TIMER_START;
        err = H5VLfile_optional (fp->uo, fp->uvlid, args, dxpl_id, req);
        CHECK_ERR
        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VLFILE_OPTIONAL);

        if (fp->is_log_based_file) {
            if(args->op_type == H5VL_NATIVE_FILE_POST_OPEN) {
                if (!(fp->lgp)) {  // Log group is already set for file create
                    H5VL_log_filei_post_open (fp);
                }
                
            }
        }
        H5VL_LOGI_PROFILING_TIMER_STOP (fp, TIMER_H5VL_LOG_FILE_OPTIONAL);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
} /* end H5VL_log_file_optional() */

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_file_close
 *
 * Purpose:     Closes a file.
 *
 * Return:      Success:    0
 *              Failure:    -1, file not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t H5VL_log_file_close (void *file, hid_t dxpl_id, void **req) {
    herr_t err = 0;
    H5VL_log_file_t *fp;
    try {
        fp = (H5VL_log_file_t *)file;
        H5VL_log_filei_dec_ref (fp);
    }
    H5VL_LOGI_EXP_CATCH_ERR

err_out:;
    return err;
} /* end H5VL_log_file_close() */
