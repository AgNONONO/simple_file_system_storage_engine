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
	
	
	//2. 读取文件metainfo
	uint64_t file_id = 0;
	cout<<"Type your file id:"<<endl;
	cin >> file_id;
	// 合法判断
	if(block_id<1){
		cerr<<"Invalid file_id, exit"<<endl;
		exit(-1);
	}
	
	nmsp_large_file::MetaInfo meta;
	ret = index_handle->read_segment_meta(file_id, meta);
	if(ret!=nmsp_large_file::TFS_SUCCESS){
		fprintf(stderr,"read_segment_meta error. file id: %lld, ret: %d\n", file_id, ret);
		exit(-3);
	}
	
	//3. 根据meta info读取文件
	std::stringstream tmp_stream;
	tmp_stream << "." << nmsp_large_file::MAINBLOCK_DIR_PREFIX<<block_id;
	tmp_stream >> mainblock_path;
	
	nmsp_large_file::FileOperation* mainblock = new nmsp_large_file::FileOperation(mainblock_path,O_RDWR);
	
	char* buf = new char[meta.get_size() + 1];
	
	ret = mainblock->pread_file(buf, meta.get_size(), meta.get_offset());
	if(ret!=nmsp_large_file::TFS_SUCCESS){
		fprintf(stderr,"mainblock->pread_file error. ret: %d error: %s\n", ret, strerror(errno));
		mainblock->close_file();
		delete mainblock;
		delete index_handle;
		exit(-4);
	}
	buf[meta.get_size() + 1] = '\0';
	cout<<"read size: "<<meta.get_size()<<"\t"<<buf<<endl;
	
	// 其他
	mainblock->close_file();
	delete mainblock;
	delete index_handle;
	
	return 0;
}