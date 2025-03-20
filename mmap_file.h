#ifndef _MMAP_FILE_H_
#define _MMAP_FILE_H_

#include <iostream>

#include <sys/mman.h>
#include "common.h"

namespace nmsp_fsse{
    namespace nmsp_large_file{
	
	// 内存映射类
	class MMapFile{
	    public:
	      MMapFile();
	      explicit MMapFile(const int fd); // 显示构造，防止赋值隐式转换
	      MMapFile(const MMapOption& mmap_option, const int fd); 
	      
	      ~MMapFile();

	      bool sync_file(); //同步文件
	      bool map_file(const bool write = false); //文件映射到内存，设置可读可写
	      void* get_data() const; // 获取映射到内存的数据的首地址
	      int32_t get_size() const; // 映射了多大 即数据大小
	      bool munmap_file(); //解除映射
	      bool remap_file(); //重新映射,增加内存


	    private:
	      bool ensure_file_size(const int32_t size); // 调整映射大小
  	      
	      int32_t size_;	// 映射文件的大小
	      int fd_;
	      void* data_;
	      struct MMapOption mmap_file_option_;

	};
    }

}

#endif // _MMAP_FILE_H_
