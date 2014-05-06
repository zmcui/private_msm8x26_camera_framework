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
}
