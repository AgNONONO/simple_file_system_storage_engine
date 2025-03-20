#ifndef MMAP_FILE_OP_H_
#define MMAP_FILE_OP_H_

#include "common.h"
#include "file_op.h"
#include "mmap_file.h"

namespace nmsp_fsse{
	namespace nmsp_large_file{
		
		// 内存映射操作类
		class MMapFileOperation: public FileOperation{
		public:
			MMapFileOperation(const std::string& file_name,const int open_flags = O_RDWR|O_LARGEFILE|O_CREAT): FileOperation(file_name,open_flags), map_file_(NULL), is_mapped_(false){
				
			}
			
			~MMapFileOperation(){
				if(map_file_){
					delete(map_file_);
					map_file_ = NULL;
				}
			}
			
			int pread_file(char* buf, const int32_t size, const int64_t offset);
			int pwrite_file(const char* buf, const int32_t size, const int64_t offset);
			
			int mmap_file(const MMapOption& mmap_option); // 根据可选项，将map_file_映射到内存
			int munmap_file();
			
			void* get_map_data()const;
			int flush_file(); // 内存写入磁盘
			
		private:
			MMapFile* map_file_; // 被映射的文件
			bool is_mapped_; // 记录此文件是否已经被映射
			
			
		};
	}
}



#endif // MMAP_FILE_OP_H_