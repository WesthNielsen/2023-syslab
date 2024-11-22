/* Error reporting */
int error_args_fatal ( int argc, char **argv );
int error_socket_fatal ( int returncode );
int error_socket_option( int returncode );
int error_socket_server( int server_fd );
int error_bind_fatal ( int returncode );
int error_listen_fatal ( int returncode );
int error_accept_fatal ( int returncode );
int error_accept ( int returncode );
int error_close ( int returncode );
int error_read ( int returncode );
int error_header ( int return_cd );
int error_non_get ( char *method );
int error_write_server ( int server_fd, int return_cd );
int error_write_client ( int client_fd, int n ); 
int error_read_server ( int server_fd, int n );
int error_close_server ( int returncode );
int error_address_server ( int return_cd );
