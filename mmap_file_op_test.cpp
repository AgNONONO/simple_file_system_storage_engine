#include "mmap_file_op.h"
#include "common.h"

using namespace std;
using namespace nmsp_tfs;

const static nmsp_large_file::MMapOption mmap_option = {10240000,4096,4096}; // 内存映射的参数

int main(){
	int ret = 0;
	const char* filename = "mmap_file_op.txt";
	nmsp_large_file::MMapFileOperation* mmfo = new nmsp_large_file::MMapFileOperation(filename);
	
	// 打开文件
	int fd = mmfo->open_file();
	if(fd < 0){
		fprintf(stderr,"open file %s failed reason: %s \n",filename, strerror(-fd));
		exit(-1);
	}
	
	ret = mmfo->mmap_file(mmap_option);
	if(ret==nmsp_large_file::TFS_ERROR){
		fprintf(stderr,"mmap_file failed reason: %s \n", strerror(-fd));
		mmfo->close_file();
		exit(-2);
	}
	
	// 写
	char buf[128+1];
	memset(buf,'1',128);
	
	ret = mmfo->pwrite_file(buf,128,8);
	if(ret < 0){
		if(ret==nmsp_large_file::EXIT_DISK_OPER_INCOMPLETE){
			fprintf(stderr,"pwrite_file: write length is less than required\n");
		}else{
			fprintf(stderr,"pwrite file %s failed, reason: %s\n", filename, strerror(-ret));
		}
	}
	
	// 读
	memset(buf,0,128);
	ret = mmfo->pread_file(buf, 128, 8);
	if(ret < 0){
		if(ret==nmsp_large_file::EXIT_DISK_OPER_INCOMPLETE){
			fprintf(stderr,"pread_file: read length is less than required\n");
		}else{
			fprintf(stderr,"pread file %s failed, reason: %s\n", filename, strerror(-ret));
		}
	}
	else{
		buf[128] = '\0';
		printf("read: %s\n", buf);
	}
	// 同步到磁盘文件
	ret = mmfo->flush_file();
	if(ret == nmsp_large_file::TFS_ERROR){
		fprintf(stderr,"flush file failed: reason: %s\n", strerror(errno));
	}
	
	
	ret = mmfo->pwrite_file(buf, 128, 4000);
	
	// 解除映射
	mmfo->munmap_file();
	
	// 关闭文件
	mmfo->close_file();
	
	return 0;
}