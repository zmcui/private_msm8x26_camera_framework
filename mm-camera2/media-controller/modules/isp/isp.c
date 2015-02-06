/*
 * isp_start_session
 *  @isp: isp instance
 *  @session_id: session id to be started
 *
 * Starts new isp session, new instance of ion driver, zoom
 * session, buffer manager, async task thread.
 *
 * Returns o for sucess and negative error for failure
 * */
int isp_start_session(isp_t *isp, uint32_t session_id)
{
  int i;
  isp_session_t *session = NULL;

  ISP_DBG(ISP_MOD_COM, "%s: E, isp %p, session_id %d", __func__, (void *)isp, session_id);
  for(i = 0; i < ISP_MAX_SESSIONS; i++){
    ...

    /* open tintless session */
    session->tintless_session = isp_tintless_open_session(isp->data.tintless,
        session->session_id);
    if(session->tintless_session == NULL) {
      CDBG_ERROR("%s: cannot open tintless session\n", __func__)
    }

    /* open zoom session */
    session->zoom_session = isp_zoom_open_session(isp->data.zoom,
        session->session_id);
    if(session->zoom_session == NULL) {
      CDBG_ERROR("%s: cannot open zoom session\n", __func__)
      return -1;
    }

    ...
  }
}
