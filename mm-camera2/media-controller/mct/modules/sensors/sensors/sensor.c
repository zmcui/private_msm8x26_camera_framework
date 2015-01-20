/*===============================================
 * FUNCTION   - sensor_get_capabilities
 *
 * DESCRIPTION:
 *===============================================*/
static int32_t sensor_get_capabilities(void *slib, void *data)
{
  int32_t rc = SENSOR_SUCCESS;
  sensor_lib_params_t *lib = (sensor_lib_params_t *)slib;
  mct_pipeline_sensor_cap_t *sensor_cap = (mct_pipeline_sensor_cap_t *)data;
  uint32_t i = 0, size = 0;
  struct sensor_lib_out_info_t *out_info = NULL;

  ...

  if (lib->sensor_lib_ptr->sensor_output->output_format == SENSOR_YCBCR) {
    sensor_cap->ae_lock_supported = FALSE;
    sensor_cap->wb_lock_supported = FALSE;
    sensor_cap->scene_mode_supported = FALSE;
    /* scene mode for yuv sensor */
    if (lib->sensor_lib_ptr->sensor_supported_scene_mode) {
      sensor_cap->sensor_supported_scene_modes = *(lib->sensor_support_scene_mode);
    }
    /* effect mode for yuv sensor */
    if (lib->sensor_lib_ptr->sensor_supported_effect_mode) {
      sensor_cap->sensor_supported_effect_modes = *(lib->sensor_support_effect_mode);
    }
  } else {
    sensor_cap->ae_lock_supported = TRUE;
    sensor_cap->wb_lock_supported = TRUE;
    sensor_cap->scene_mode_supported = TRUE;
  }

  ...
}


/*===============================================
 * FUNCTION   - sensor_process 
 *
 * DESCRIPTION:
 *===============================================*/
static int32_t sensor_process(void *sctrl,
    sensor_submodule_event_type_t event, void *data)
{
  int32_t rc = SENSOR_SUCCESS;
  if(!sctrl) {
    SERR("failed");
    return SENSOR_FAILURE;
  }

  SLOW("sctrl %p event %d", sctrl, event);
  switch (event) {
  /* get enums */
  case SENSOR_GET_CAPABILITIES:
    rc = sensor_get_capabilities(sctrl, data);
    break;
  case SENSOR_GET_CUR_CSIPHY_CFG:
  ...
  default:
    SERR("invalid event %d", event);
    rc = SENSOR_FAILURE;
    break;
  }
  return rc;
}
