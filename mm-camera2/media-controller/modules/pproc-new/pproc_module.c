/*
 * _pproc_module_private:
 *  @cpp_module:  cpp submodule
 *  @vpe_module:  vpe submodule
 *  @c2d_module:  c2d submodule
 *  @cac_module:  cac submodule
 *  @wnr_module:  wnr submodule
 *  @module_type_list: list to hold identity(stream) and
 *                     corresponding module type
 *
 * private object structure for pproc module
 * */
typedef struct _pproc_module_private {
  mct_module_t *cpp_module;
  mct_module_t *vpe_module;
  mct_module_t *c2d_module;
  mct_module_t *cac_module;
  mct_module_t *wnr_module;
  mct_list_t *module_type_list;
} pproc_module_private_t

/*
 * pproc_module_query_mod
 *  @module:    pproc module itself ("pproc")
 *  @query_buf: media controller's query information buffer
 *  @sessionid: session and stream identiry
 *
 * pproc module's capability is based on the sub-modules and
 * independant of the session id. this function is used to
 * query submodule
 * */
static boolean pproc_module_query_mod(mct_module_t *module,
    void *buf, unsigned int sessionid)
{
  pproc_module_private_t *mod_private;
  boolean rc = FALSE;
  mct_pipeline_cap_t *query_buf = (mct_pipeline_cap_t *)buf;

  if(!module || !query_buf || strcmp(MCT_OBJECT_NAME(module), "pproc")) {
    CDBG_HIGH("%s:%d] error module: %p query_buf: %p\n", __func__, __LINE__,
        module, query_buf);
    return r;
  }

  MCT_OBJECT_LOCK(module);
  mod_private = (pproc_module_private_t *)MCT_OBJECT_PRIVATE(module);

  /* TODO: We should probably independently store the sub-mods caps and give
   * the aggregate caps to media-controler */

  memset(&query_buf->pp_cap, 0, sizeof(mct_pipeline_cap_t));
  //cpp 
  if (mod_private->cpp_module && mod_private->cpp_module->query_mod) {
    if (mod_private->cpp_module->query_mod(mod_private->cpp_module, query_buf,
          sessionid) == FALSE) {
      CDBG_ERROR("%s:%d] error in cpp query mod\n", __func__, __LINE__);
      goto query_mod_done;
    }
  }

  //vpe
  if (mod_private->vpe_module && mod_private->vpe_module->query_mod) {
    if (mod_private->vpe_module->query_mod(mod_private->vpe_module, query_buf,
          sessionid) == FALSE) {
      CDBG_ERROR("%s:%d] error in vpe query mod\n", __func__, __LINE__);
      goto query_mod_done;
    }
  }

  //c2d
  if (mod_private->c2d_module && mod_private->c2d_module->query_mod) {
    if (mod_private->c2d_module->query_mod(mod_private->c2d_module, query_buf,
          sessionid) == FALSE) {
      CDBG_ERROR("%s:%d] error in c2d query mod\n", __func__, __LINE__);
      goto query_mod_done;
    }
  }

  //cac
  if (mod_private->cac_module && mod_private->cac_module->query_mod) {
    if (mod_private->cac_module->query_mod(mod_private->cac_module, query_buf,
          sessionid) == FALSE) {
      CDBG_ERROR("%s:%d] error in cac query mod\n", __func__, __LINE__);
      goto query_mod_done;
    }
  }

  //wnr
  if (mod_private->wnr_module && mod_private->wnr_module->query_mod) {
    if (mod_private->wnr_module->query_mod(mod_private->wnr_module, query_buf,
          sessionid) == FALSE) {
      CDBG_ERROR("%s:%d] error in wnr query mod\n", __func__, __LINE__);
      goto query_mod_done;
    }
  }

  rc = TRUE;

query_mod_done:
  MCT_OBJECT_UNLOCK(module);
  CDBG_HIGH("%s:%d] feature_mask 0x%x X\n", __func__, __LINE__,
      query_buf->pp_cap.feature_mask);
  return rc;
}

/*
 * pproc_module_init:
 *  @name: name of this pproc interface module("proc")
 *
 * pproc interface module initialization entry point, it only
 * creates pproc module and initialize its sub-modules(cpp/vpe,
 * c2d, s/w scaling modules etc.). pproc should also initialize
 * its sink and source ports which map to its sub-modules ports.
 *
 * Return: pproc module object if success
 * */
mct_module_t* pproc_module_init(const char *name)
{
  int i;
  mct_module_t *pproc;
  mct_port_t *port;
  pproc_module_private_t *mod_private;

  if (strcmp(name, "pproc")) {
    CDBG_HIGH("%s:%d] invalid module name\n", __func__, __LINE__);
    return NULL;
  }

  pproc = mct_module_create("pproc");
  if(!pproc) {
    CDBG_ERROR("%s:%d] error module create failed\n", __func__, __LINE__);
    return NULL;
  }

  mod_private = malloc(sizeof(pproc_module_private_t));
  if(mod_private == NULL){
    CDBG_ERROR("%s:%d] error because private data alloc failed\n", __func__, __LINE__);
    goto private_error
  }

  memset(mod_private, 0, sizeof(pproc_module_private_t));

  /* TODO: Add version or caps based information to build topology */
  mod_private->cpp_module = cpp_module_init("cpp");
  mod_private->vpe_module = vpe_module_init("vpe");
  mod_private->c2d_module = c2d_module_init("c2d");
#if CAMERA_FEATURE_CAC
  mod_private->cac_module = cac_module_init("cac");
  if (NULL != mod_private->cac_module)
    /* populate the parent pointer */
    module_cac_set_parent(mod_private->cac_module, pproc);
  else
    CDBG_ERROR("%s:%d] cac module create failed\n", __func__, __LINE__);
#else
#if CAMERA_FEATURE_WNR_SW
  mod_private->wnr_module = wnr_module_init("wnr");
  if (NULL != mod_private->wnr_module)
    /* populate the parent pointer */
    module_wnr_set_parent(mod_private->wnr_module, pproc);
  else
    CDBG_ERROR("%s:%d] wnr module create failed\n", __func__, __LINE__);
#endif
#endif
  ...

  mct_module_set_query_mod_func(pproc, pproc_module_query_mod);
  ...
}
