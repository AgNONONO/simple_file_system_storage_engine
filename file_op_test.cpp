#include "file_op.h"
#include "common.h"

using namespace std;
using namespace nmsp_tfs;

int main(){
	const char* filename = "file_op.txt";
	nmsp_large_file::FileOperation* fileOP = new nmsp_large_file::FileOperation(filename,O_CREAT|O_RDWR|O_LARGEFILE);
	
	int fd = fileOP->open_file();
	if(fd < 0){
		fprintf(stderr,"open file %s failed, reason: %s\n", filename, strerror(-fd));
		exit(-1);
	}
	char buf[65];
	memset(buf,'8',64);
	int ret = fileOP->pwrite_file(buf,64,128);
	if(ret < 0){
		if(ret==nmsp_large_file::EXIT_DISK_OPER_INCOMPLETE){
			fprintf(stderr,"pwrite_file: write length is less than required\n");
		}else{
			fprintf(stderr,"pwrite file %s failed, reason: %s\n", filename, strerror(-ret));
		}
	}
	
	memset(buf,0,64);
	ret = fileOP->pread_file(buf, 64, 128);
	if(ret < 0){
		if(ret==nmsp_large_file::EXIT_DISK_OPER_INCOMPLETE){
			fprintf(stderr,"pread_file: read length is less than required\n");
		}else{
			fprintf(stderr,"pread file %s failed, reason: %s\n", filename, strerror(-ret));
		}
	}
	else{
		buf[64] = '\0';
		printf("read: %s\n", buf);
	}
	
	ret = fileOP->write_file(buf,64);
	if(ret < 0){
		if(ret==nmsp_large_file::EXIT_DISK_OPER_INCOMPLETE){
			fprintf(stderr,"write_file: write length is less than required\n");
		}else{
			fprintf(stderr,"write file %s failed, reason: %s\n", filename, strerror(-ret));
		}
	}
	
	
	fileOP->close_file();
	
	return 0;
}