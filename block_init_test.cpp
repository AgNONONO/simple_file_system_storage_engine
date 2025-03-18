#include "index_handle.h"
#include "common.h"
#include "file_op.h"
#include <sstream>

using namespace std;
using namespace nmsp_tfs;

const static nmsp_large_file::MMapOption mmap_option={1024000,4096,4096};
const static uint32_t main_blocksize = 1024*1024*64; // 主块大小
const static uint32_t bucket_size = 1000; // 哈希桶大小
static int32_t block_id = 1;
static int debug = 1;


int main(int argc, char** argv){ // argv[0] = 'rm' argv[1] = '-f' argv[1] = 'a.out' 
	std::string mainblock_path;
	std::string index_path;
	int32_t ret = nmsp_large_file::TFS_SUCCESS;
	
	cout<<"Type your blockid:"<<endl;
	cin >> block_id;
	if(block_id<1){
		cerr<<"Invalid block_id, exit"<<endl;
		exit(-1);
	}
	
	// 1. 创建索引
	nmsp_large_file::IndexHandle* index_handle = new nmsp_large_file::IndexHandle(".",block_id); 
	// 索引文件句柄
	
	if(debug) printf("Init index...\n");
	
	ret = index_handle->create(block_id,bucket_size,mmap_option);
	
	if(ret!=nmsp_large_file::TFS_SUCCESS){
		
		fprintf(stderr,"create index  %d failed\n", block_id);
		delete index_handle;
		exit(-3);
	}
	
	//2. 生成主块
	std::stringstream tmp_stream;
	tmp_stream << "." << nmsp_large_file::MAINBLOCK_DIR_PREFIX<<block_id;
	tmp_stream >> mainblock_path;
	
	nmsp_large_file::FileOperation* mainblock = new nmsp_large_file::FileOperation(mainblock_path,O_RDWR|O_LARGEFILE|O_CREAT);
	
	ret = mainblock->ftruncate_file(main_blocksize);
	
	if(ret!=0){
		fprintf(stderr,"create main block %s failed, reason: %s\n", mainblock_path.c_str(),strerror(errno));
		delete mainblock;
		delete index_handle;
		index_handle->remove(block_id);
		exit(-2);
	}
	
	// 其他
	index_handle->flush();
	mainblock->close_file();

	
	delete mainblock;
	delete index_handle;
	
	return 0;
}