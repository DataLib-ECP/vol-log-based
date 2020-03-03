#include "logvol_internal.hpp"

/********************* */
/* Function prototypes */
/********************* */
void *H5VL_log_dataset_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id, hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req);
void *H5VL_log_dataset_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t dapl_id, hid_t dxpl_id, void **req);
herr_t H5VL_log_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, void *buf, void **req);
herr_t H5VL_log_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t plist_id, const void *buf, void **req);
herr_t H5VL_log_dataset_get(void *dset, H5VL_dataset_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_dataset_specific(void *obj, H5VL_dataset_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_dataset_optional(void *obj, H5VL_dataset_optional_t optional_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_dataset_close(void *dset, hid_t dxpl_id, void **req);

const H5VL_dataset_class_t H5VL_log_dataset_g{
    H5VL_log_dataset_create,                /* create       */
    H5VL_log_dataset_open,                  /* open         */
    H5VL_log_dataset_read,                  /* read         */
    H5VL_log_dataset_write,                 /* write        */
    H5VL_log_dataset_get,                   /* get          */
    H5VL_log_dataset_specific,              /* specific     */
    H5VL_log_dataset_optional,              /* optional     */
    H5VL_log_dataset_close                  /* close        */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_create
 *
 * Purpose:     Creates a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *H5VL_log_dataset_create(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t lcpl_id, hid_t type_id, hid_t space_id,
    hid_t dcpl_id, hid_t dapl_id, hid_t dxpl_id, void **req) {

    H5VL_log_obj_t *dset;
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    void *under;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Create\n");
#endif

    under = H5VLdataset_create(o->uo, loc_params, o->uvlid, name, lcpl_id, type_id, space_id, dcpl_id,  dapl_id, dxpl_id, req);
    if(under) {
        dset = H5VL_log_new_obj(under, o->uvlid);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_log_new_obj(*req, o->uvlid);
    } /* end if */
    else
        dset = NULL;

    return (void *)dset;
} /* end H5VL_log_dataset_create() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_open
 *
 * Purpose:     Opens a dataset in a container
 *
 * Return:      Success:    Pointer to a dataset object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_log_dataset_open(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t dapl_id, hid_t dxpl_id, void **req)
{
    H5VL_log_obj_t *dset;
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    void *under;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Open\n");
#endif

    under = H5VLdataset_open(o->uo, loc_params, o->uvlid, name, dapl_id, dxpl_id, req);
    if(under) {
        dset = H5VL_log_new_obj(under, o->uvlid);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_log_new_obj(*req, o->uvlid);
    } /* end if */
    else
        dset = NULL;

    return (void *)dset;
} /* end H5VL_log_dataset_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_read
 *
 * Purpose:     Reads data elements from a dataset into a buffer.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_dataset_read(void *dset, hid_t mem_type_id, hid_t mem_space_id,
    hid_t file_space_id, hid_t plist_id, void *buf, void **req)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)dset;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Read\n");
#endif

    ret_value = H5VLdataset_read(o->uo, o->uvlid, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    return ret_value;
} /* end H5VL_log_dataset_read() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_write
 *
 * Purpose:     Writes data elements from a buffer into a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_dataset_write(void *dset, hid_t mem_type_id, hid_t mem_space_id,
    hid_t file_space_id, hid_t plist_id, const void *buf, void **req)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)dset;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Write\n");
#endif

    ret_value = H5VLdataset_write(o->uo, o->uvlid, mem_type_id, mem_space_id, file_space_id, plist_id, buf, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    return ret_value;
} /* end H5VL_log_dataset_write() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_get
 *
 * Purpose:     Gets information about a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_dataset_get(void *dset, H5VL_dataset_get_t get_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)dset;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Get\n");
#endif

    ret_value = H5VLdataset_get(o->uo, o->uvlid, get_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    return ret_value;
} /* end H5VL_log_dataset_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_specific
 *
 * Purpose:     Specific operation on a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_dataset_specific(void *obj, H5VL_dataset_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    hid_t uvlid;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL H5Dspecific\n");
#endif

    // Save copy of underlying VOL connector ID and prov helper, in case of
    // refresh destroying the current object
    uvlid = o->uvlid;

    ret_value = H5VLdataset_specific(o->uo, o->uvlid, specific_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, uvlid);

    return ret_value;
} /* end H5VL_log_dataset_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_optional
 *
 * Purpose:     Perform a connector-specific operation on a dataset
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_dataset_optional(void *obj, H5VL_dataset_optional_t optional_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Optional\n");
#endif

    ret_value = H5VLdataset_optional(o->uo, o->uvlid, optional_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    return ret_value;
} /* end H5VL_log_dataset_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_dataset_close
 *
 * Purpose:     Closes a dataset.
 *
 * Return:      Success:    0
 *              Failure:    -1, dataset not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_dataset_close(void *dset, hid_t dxpl_id, void **req)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)dset;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL DATASET Close\n");
#endif

    ret_value = H5VLdataset_close(o->uo, o->uvlid, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->uvlid);

    /* Release our wrapper, if underlying dataset was closed */
    if(ret_value >= 0)
        H5VL_log_free_obj(o);

    return ret_value;
} /* end H5VL_log_dataset_close() */