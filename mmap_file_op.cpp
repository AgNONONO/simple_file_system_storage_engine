#include "mmap_file_op.h"
#include "common.h"
#include "file_op.h"
#include "mmap_file.h"

//static int debug = 1; 

namespace nmsp_fsse{
	namespace nmsp_large_file{
		int MMapFileOperation::mmap_file(const MMapOption& mmap_option){
			if(mmap_option.max_mmap_size_ < mmap_option.first_mmap_size_){
				return FSSE_ERROR;
			}
			
			if(mmap_option.max_mmap_size_ <= 0){
				return FSSE_ERROR;
			}
			// 打开文件
			int fd = check_file();
			if(fd < 0){
				fprintf(stderr,"MMapFileOperation::mmap_file-checking file failed");
				return FSSE_ERROR;
			}
			
			if(!is_mapped_){
				if(map_file_){
					delete(map_file_);
				}
				map_file_ = new MMapFile(mmap_option,fd);
				is_mapped_ = map_file_->map_file(true);
			}
			
			if(is_mapped_){
				return FSSE_SUCCESS;
			}
			else{
				return FSSE_ERROR;
			}
			
		}
		
		int MMapFileOperation::munmap_file(){
			if(is_mapped_ && map_file_!=NULL){
				delete(map_file_);
				is_mapped_ = false;
			}
			return FSSE_SUCCESS;
		}
		
		void* MMapFileOperation::get_map_data()const{
			if(is_mapped_){
				return map_file_->get_data();
			}
			return NULL;
		}
		
		int MMapFileOperation::pread_file(char* buf, const int32_t size, const int64_t offset){
			// 1. 内存已经映射,直接从内存读
			// if(is_mapped_ && (offset + size) > map_file_->get_size()){ //相当于扩容了一次
			while(is_mapped_ && (offset + size) > map_file_->get_size()){
				// 内存不够
				if(debug) fprintf(stdout,"MMapFileOperation: pread, size: %d, offset: %" __PRI64_PREFIX "d, map file size: %d. need remap\n", size, offset, map_file_->get_size());
				
				map_file_->remap_file();
			}
			// while多扩几次
			
			if(is_mapped_ && (offset + size) <= map_file_->get_size()){
				memcpy(buf,(char*)map_file_->get_data()+offset,size);
				return FSSE_SUCCESS;
			}
			
			// 2. 内存未映射 or 读取的数据映射不全
			return FileOperation::pread_file(buf,size,offset);
			
		}
		
		
		int MMapFileOperation::pwrite_file(const char* buf, const int32_t size, const int64_t offset){
			// 1. 内存已经映射,直接往内存写
			// if(is_mapped_ && (offset + size) > map_file_->get_size()){
			while(is_mapped_ && (offset + size) > map_file_->get_size()){
				// 内存不够
				if(debug) fprintf(stdout,"MMapFileOperation: pwrite, size: %d, offset: %" __PRI64_PREFIX "d, map file size: %d. need remap\n", size, offset, map_file_->get_size());
				
				map_file_->remap_file();
			}
			
			if(is_mapped_ && (offset + size) <= map_file_->get_size()){
				// pwrite 相当于 lseek + write，而 write 是直接写到内核空间中的缓冲区，相当于直接写入了文件
				memcpy((char*)map_file_->get_data()+offset,buf,size);
				return FSSE_SUCCESS;
			}
			
			// 2. 内存未映射 or 读取的数据映射不全
			return FileOperation::pwrite_file(buf,size,offset);
		}
		
		int MMapFileOperation::flush_file(){
			if(is_mapped_){
				if(map_file_->sync_file()){
					return FSSE_SUCCESS;
				}
				else{
					return FSSE_ERROR;
				}
			}
			// 没映射
			return FileOperation::flush_file();
		}
		
	}
}