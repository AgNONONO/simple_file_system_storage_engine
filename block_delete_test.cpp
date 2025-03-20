#include "index_handle.h"
#include "common.h"
#include "file_op.h"
#include <sstream>

using namespace std;
using namespace nmsp_fsse;

const static nmsp_large_file::MMapOption mmap_option={1024000,4096,4096};
const static uint32_t main_blocksize = 1024*1024*64; // 主块大小
const static uint32_t bucket_size = 1000; // 哈希桶大小
static int32_t block_id = 1;
static int debug = 1;


int main(int argc, char** argv){ // argv[0] = 'rm' argv[1] = '-f' argv[1] = 'a.out' 
	std::string mainblock_path;
	std::string index_path;
	int32_t ret = nmsp_large_file::FSSE_SUCCESS;
	
	cout<<"Type your blockid:"<<endl;
	cin >> block_id;
	if(block_id<1){
		cerr<<"Invalid block_id, exit"<<endl;
		exit(-1);
	}
	
	// 1. 加载索引
	nmsp_large_file::IndexHandle* index_handle = new nmsp_large_file::IndexHandle(".",block_id); 
	// 索引文件句柄
	
	if(debug) printf("Load index...\n");
	
	ret = index_handle->load(block_id,bucket_size,mmap_option);
	
	if(ret!=nmsp_large_file::FSSE_SUCCESS){
		
		fprintf(stderr,"load index  %d failed\n", block_id);
		delete index_handle;
		exit(-2);
	}
	
	
	//2. 读取要删除的文件metainfo
	uint64_t file_id = 0;
	cout<<"Type your file id:"<<endl;
	cin >> file_id;
	// 合法判断
	if(block_id<1){
		cerr<<"Invalid file_id, exit"<<endl;
		delete index_handle;
		exit(-1);
	}
	
	ret = index_handle->delete_segment_meta(file_id);
		
	if(ret!=nmsp_large_file::FSSE_SUCCESS){
		
		fprintf(stderr,"delete_segment_meta failed. file id: %lld, ret: %d\n", file_id, ret);
		delete index_handle;
		exit(-3);
	}
	
	ret = index_handle->flush();
	if(ret!=nmsp_large_file::FSSE_SUCCESS){
		fprintf(stderr,"flush block %d failed, file no: %d, reason: %s\n", block_id, file_id, strerror(errno));
		delete index_handle;
		exit(-4);
	}
	
	printf("delete successfully\n");
	delete index_handle;
	
	return 0;
}