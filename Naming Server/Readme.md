# Assumptions

- The backup server is running on the same machine as the naming server.
- The backup is done by copying the files from the naming server to the backup server.
- When the main storage server is down, only READ operations are allowed.
- And when READ operations are allowed, the naming server will return the IP address and port number of the backup server and also mentioning that which folder to be used to get the files.
- The folder is mentioned in the request message sent back to the client.
- The syntax of the request message to get the folder where it is backed is as follows:
  - "Backup | folder_number"
- The folder number is the number of the folder where the files are backed up.
- The client can access this folder to READ the files of the storage server that is down.
- In CREATE and DELETE operations, the structure is as follows:
  - src_path stores the path where the file is to be created or deleted.
  - src_file_dir_name stores the file or the directory name to be created or deleted.
  - req->req_type stores the type of operation to be performed.
In the copy operation, the structure is as follows:
  - src_path stores the path of the file to be copied.
  - src_file_dir_name stores the file name to be copied.
  - dest_path stores the path where the file is to be copied.
  - dest_file_dir_name stores the file name to be copied.
  - req->req_type stores the type of operation to be performed.
  - requests are sent to both the src and dest storage servers.
  - the request sent provides the src storage server with IP address and port number of the dest storage server and vice versa.
