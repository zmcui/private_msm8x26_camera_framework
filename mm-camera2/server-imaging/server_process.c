static mct_module_init_name_t modules_list[] = {
	{"sensor", module_sensor_init, module_sensor_deinit},
	{"iface", module_iface_init, module_iface_deinit},
	{"isp", module_isp_init, module_isp_deinit},
	{"stats", stats_module_init, stats_module_deinit},
	{"pproc", pproc_module_init, pproc_module_deinit},
	{"imglib", module_imglib_init, module_imglib_deinit},
};

/** server_process_module_init:
 *
 *  Very first thing Imaging Server performs after it starts
 *  to build up module list. One specific module initilization
 *  may not success because the module may not exist on this
 *  platform.
 **/
boolean server_process_module_init(void)
{
	mct_module_t *temp = NULL;
	int i;
	
	/* we could see this log at very first, e.g.
	01-21 07:50:23.850: E/mm-camera(286): server_process_module_init:59, int mods*/
	CDBG_ERROR("%s:%d, int mods", __func__, __LINE__);
	
	for(i = 0; i < (int)(sizeof(modules_list)/sizeof(mct_module_init_name_t)); i++){
		if(NULL == modules_list[i].init_mod)
			continue;
		
		temp = modules_list[i].init_mod(modules_list[i].name);
		if(temp){
			if((modules = mct_list_append(modules, temp, NULL, NULL)) == NULL){
				if(modules){
					for(i--; i >= 0; i--)
						modules_list[i].deinit_mod(temp);
					
					mct_list_free_all(modules, NULL);
					return FALSE;
				}
			}
		}
	}/*for*/
	
	return TRUE;
}
