/*
 * module_sensor_query_mod: query mod function for sensor
 * module
 *
 * @query_buf: pointer to module_sensor_query_caps_t struct
 * @session: session_id
 * @s_module: mct module pointer for sensor
 *
 * Return: 0 for success and negative error for failure
 *
 * This function handles query module events to return
 * information requested by mct any stream is created
 * */
static boolean module_sensor_query_mod(mct_module_t *module,
    void *buf, unsigned int sessionid)
{
  int32_t idx = 0, rc = SENSOR_SUCCESS;
  mct_pipeline_sensor_cap_t *sensor_cap = NULL;
  ...

  //goto sensors/sensors/sensor.c
  rc = module_sensor_params->func_tbl.process(s_bundle->sensor_lib_params,
      SENSOR_GET_CAPABILITIES, sensor_cap)
  ...
}
