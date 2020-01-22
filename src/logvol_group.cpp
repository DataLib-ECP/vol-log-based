#include "logvol.h"


/********************* */
/* Function prototypes */
/********************* */

void *H5VL_log_group_create(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id, hid_t dxpl_id, void **req);
void *H5VL_log_group_open(void *obj, const H5VL_loc_params_t *loc_params, const char *name, hid_t gapl_id, hid_t dxpl_id, void **req);
herr_t H5VL_log_group_get(void *obj, H5VL_group_get_t get_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_group_specific(void *obj, H5VL_group_specific_t specific_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_group_optional(void *obj, H5VL_group_optional_t opt_type, hid_t dxpl_id, void **req, va_list arguments);
herr_t H5VL_log_group_close(void *grp, hid_t dxpl_id, void **req);

const H5VL_group_class_t H5VL_log_group_g{
    H5VL_log_group_create,                /* create       */
    H5VL_log_group_open,                  /* open       */
    H5VL_log_group_get,                   /* get          */
    H5VL_log_group_specific,              /* specific     */
    H5VL_log_group_optional,              /* optional     */
    H5VL_log_group_close                  /* close        */
};

/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_create
 *
 * Purpose:     Creates a group inside a container
 *
 * Return:      Success:    Pointer to a group object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_log_group_create(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id,
    hid_t dxpl_id, void **req)
{
    H5VL_log_obj_t *group;
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    void *under;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL GROUP Create\n");
#endif

    under = H5VLgroup_create(o->under_object, loc_params, o->under_vol_id, name, lcpl_id, gcpl_id,  gapl_id, dxpl_id, req);
    if(under) {
        group = H5VL_log_new_obj(under, o->under_vol_id);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_log_new_obj(*req, o->under_vol_id);
    } /* end if */
    else
        group = NULL;

    return (void *)group;
} /* end H5VL_log_group_create() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_open
 *
 * Purpose:     Opens a group inside a container
 *
 * Return:      Success:    Pointer to a group object
 *              Failure:    NULL
 *
 *-------------------------------------------------------------------------
 */
void *
H5VL_log_group_open(void *obj, const H5VL_loc_params_t *loc_params,
    const char *name, hid_t gapl_id, hid_t dxpl_id, void **req)
{
    H5VL_log_obj_t *group;
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    void *under;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL GROUP Open\n");
#endif

    under = H5VLgroup_open(o->under_object, loc_params, o->under_vol_id, name, gapl_id, dxpl_id, req);
    if(under) {
        group = H5VL_log_new_obj(under, o->under_vol_id);

        /* Check for async request */
        if(req && *req)
            *req = H5VL_log_new_obj(*req, o->under_vol_id);
    } /* end if */
    else
        group = NULL;

    return (void *)group;
} /* end H5VL_log_group_open() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_get
 *
 * Purpose:     Get info about a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_group_get(void *obj, H5VL_group_get_t get_type, hid_t dxpl_id,
    void **req, va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL GROUP Get\n");
#endif

    ret_value = H5VLgroup_get(o->under_object, o->under_vol_id, get_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->under_vol_id);

    return ret_value;
} /* end H5VL_log_group_get() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_specific
 *
 * Purpose:     Specific operation on a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_group_specific(void *obj, H5VL_group_specific_t specific_type,
    hid_t dxpl_id, void **req, va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    hid_t under_vol_id;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL GROUP Specific\n");
#endif

    // Save copy of underlying VOL connector ID and prov helper, in case of
    // refresh destroying the current object
    under_vol_id = o->under_vol_id;

    ret_value = H5VLgroup_specific(o->under_object, o->under_vol_id, specific_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, under_vol_id);

    return ret_value;
} /* end H5VL_log_group_specific() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_optional
 *
 * Purpose:     Perform a connector-specific operation on a group
 *
 * Return:      Success:    0
 *              Failure:    -1
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_group_optional(void *obj, H5VL_group_optional_t opt_type, hid_t dxpl_id, void **req,
    va_list arguments)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)obj;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL GROUP Optional\n");
#endif

    ret_value = H5VLgroup_optional(o->under_object, o->under_vol_id, opt_type, dxpl_id, req, arguments);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->under_vol_id);

    return ret_value;
} /* end H5VL_log_group_optional() */


/*-------------------------------------------------------------------------
 * Function:    H5VL_log_group_close
 *
 * Purpose:     Closes a group.
 *
 * Return:      Success:    0
 *              Failure:    -1, group not closed.
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5VL_log_group_close(void *grp, hid_t dxpl_id, void **req)
{
    H5VL_log_obj_t *o = (H5VL_log_obj_t *)grp;
    herr_t ret_value;

#ifdef ENABLE_PASSTHRU_LOGGING 
    printf("------- LOG VOL H5Gclose\n");
#endif

    ret_value = H5VLgroup_close(o->under_object, o->under_vol_id, dxpl_id, req);

    /* Check for async request */
    if(req && *req)
        *req = H5VL_log_new_obj(*req, o->under_vol_id);

    /* Release our wrapper, if underlying file was closed */
    if(ret_value >= 0)
        H5VL_log_free_obj(o);

    return ret_value;
} /* end H5VL_log_group_close() */