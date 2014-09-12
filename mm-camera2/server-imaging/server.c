/** main
 *
 **/
int main(int argc, char *argv[])
{
....

/* 2.after open node, initialize modules */
if(server_process_module_init() == FALSE)
	goto module_init_fail;
....

	do{
		FD_ZERO(&(select_fds.fds));
		mct_list_traverse(listen_fd_list, server_reset_select_fd, &select_fds);
		
		/* no timeout */
		ret = select(select_fds.select_fd + 1, &(select_fds.fds), NULL, NULL, NULL);
		
		if(ret > 0){
			
			mct_list_t		*find_list;
			read_fd_info_t	*fd_info;
			
			find_list = mct_list_find_custom(listen_fd_list, &(select_fds.fds),
				server_check_listen_fd);
			if(!find_list)
				continue;
			
			fd_info = (read_fd_info_t *)find_list->data;
			
			switch(fd_info->type){
				case RD_FD_HAL:{
					if(ioctl(fd_info->fd[0], VIDIOC_DQEVENT, &event) < 0)
						coutinue;
						
					/* server process HAL event:
					 * 
					 *	1. if it returns success, it means the event message has been
					 *	   posted to MCT, don't need to send CMD ACK back to kernel
					 *	   immediately, because MCT will notify us after process;
					 *	2. if it returns failure, it means the event message was not
					 *	   posted to MCT successfully, hence we need to send CMD ACK back
					 *	   to kernel immediatelly so that HAL thread which sends this
					 *	   event can be blocked
					 */
					proc_ret = server_process_hal_event(&event);
				}
					break;
					
				case RD_DS_FD_HAL:
					/* server process message sent by HAL through Domain Socket */
					proc_ret = server_process_hal_ds_package(fd_info->fd[0],
						fd_info->session);
					break;
					
				case RD_PIPE_FD_MCT:
					/* server process message sent by media controller
					 * through pipe: */
					proc_ret = server_process_mct_msg(fd_info->fd[0],
						fd_info->session);
					break;
					
				default:
					continue;
			} /* switch (fd_info->type)*/
			....
			
		}else{
			/* select failed. it cannot time out. */	
			/* To Do: handle error*/
		}
	}while(1)

}
