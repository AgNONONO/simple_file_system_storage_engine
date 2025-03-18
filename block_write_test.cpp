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
	
	// 1. 加载索引
	nmsp_large_file::IndexHandle* index_handle = new nmsp_large_file::IndexHandle(".",block_id); 
	// 索引文件句柄
	
	if(debug) printf("Load index...\n");
	
	ret = index_handle->load(block_id,bucket_size,mmap_option);
	
	if(ret!=nmsp_large_file::TFS_SUCCESS){
		
		fprintf(stderr,"load index  %d failed\n", block_id);
		delete index_handle;
		exit(-2);
	}
	
	//2. 文件写入到主块
	std::stringstream tmp_stream;
	tmp_stream << "." << nmsp_large_file::MAINBLOCK_DIR_PREFIX<<block_id;
	tmp_stream >> mainblock_path;
	
	nmsp_large_file::FileOperation* mainblock = new nmsp_large_file::FileOperation(mainblock_path,O_RDWR|O_LARGEFILE|O_CREAT);
	
	char buf[4096];
	memset(buf,'6',sizeof(buf));
	
	int32_t data_offset = index_handle->get_block_data_offset();
	uint32_t file_no = index_handle->block_info()->seq_no_;
	
	ret = mainblock->pwrite_file(buf,sizeof(buf),data_offset);
	if(ret!=nmsp_large_file::TFS_SUCCESS){
		fprintf(stderr,"write to main block failed, ret: %d, reason: %s\n", ret,strerror(errno));
		mainblock->close_file();
		
		delete mainblock;
		delete index_handle;
		exit(-3);
	}
	
	//3. 索引文件写入metaInfo
	nmsp_large_file::MetaInfo meta;
	meta.set_file_id(file_no);
	meta.set_offset(data_offset);
	meta.set_size(sizeof(buf));
	
	// meta写入哈希桶
	ret = index_handle->write_segment_meta(meta.get_key(),meta);
	if(ret == nmsp_large_file::TFS_SUCCESS){
		//1. 更新索引头信息
		index_handle->commit_block_data_offset(sizeof(buf));
		
		//2. 更新块信息
		index_handle->update_block_info(nmsp_large_file::C_OPER_INSERT, sizeof(buf));
		
		//3. 内存数据写入磁盘
		ret = index_handle->flush();
		if(ret!=nmsp_large_file::TFS_SUCCESS){
			fprintf(stderr,"flush block %d failed, file no: %d, reason: %s\n", block_id, file_no, strerror(errno));
		}
	}
	else{
		fprintf(stderr,"write_segment_meta block %d failed, file no: %d, reason: %s\n", block_id, file_no, strerror(errno));
	}
	
	if(ret!=nmsp_large_file::TFS_SUCCESS){
		fprintf(stderr,"write to main block %d failed, file no: %d, reason: %s\n", block_id, file_no, strerror(errno));
	}
	else{
		if(debug) printf("write successfully, block id: %d , file no: %d\n ", block_id, file_no);
	}
	
	// 其他
	mainblock->close_file();
	
	delete mainblock;
	delete index_handle;
	
	return 0;
}