#include "mmap_file.h"
#include "common.h"



using namespace std;
using namespace nmsp_tfs;

static const mode_t OPEN_MODE = 0644;
const static nmsp_large_file::MMapOption mmap_option = {10240000,4096,4096}; // 内存映射的参数

int open_file(string file_name,int open_flags){
	// 获得句柄
	int fd = open(file_name.c_str(),open_flags,OPEN_MODE);
	if(fd<0){
		return -errno; // errno是全局共享的，其他地方报错都会改变errno 
	}
	return fd;
}


int main(){
	// 1. 打开、创建一个文件，取得其句柄
	const char* filename = "./mapfile_test.txt";
	
	// O_LARGEFILE 用于操作大文件、超大文件
	int fd = open_file(filename,O_RDWR|O_CREAT|O_LARGEFILE);
	if(fd < 0){
		fprintf(stderr,"open file failed, filename: %s, error desc:%s",filename, strerror(-fd));
		return -1;	
	}
	
	//2. 根据句柄
	
	nmsp_large_file::MMapFile* mmapFile_obj = new nmsp_large_file::MMapFile(mmap_option,fd);
	
	bool is_mapped = mmapFile_obj->map_file(true);
	
	if(is_mapped){
		mmapFile_obj->remap_file();
		
		memset(mmapFile_obj->get_data(), '9', mmapFile_obj->get_size());
		mmapFile_obj->sync_file();
		
		mmapFile_obj->munmap_file();
	}
	else{
		fprintf(stderr,"map file failed\n");
	}
	
	close(fd);

	return 0;
}
