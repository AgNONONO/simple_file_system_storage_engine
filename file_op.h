#ifndef _FILE_OP_H_
#define _FILE_OP_H_

#include "common.h"


namespace nmsp_tfs{
	namespace nmsp_large_file{
		
		class FileOperation{
		public:
			FileOperation(const std::string &file_name, const int open_flags = O_RDWR|O_LARGEFILE);
			~FileOperation();
			
			int open_file();
			void close_file();
			
			virtual int flush_file(); // 将内存数据写入磁盘
			
			int unlink_file(); // 逻辑？删除文件
			
			virtual int pread_file(char* buf, const int32_t nbytes, const int64_t offset); // p: pre?
			virtual int pwrite_file(const char* buf, const int32_t nbytes, const int64_t offset); // seek来看偏移
			int write_file(const char* buf, const int32_t nbytes); // 从当前位置写入
			
			int64_t get_file_size();
			
			int ftruncate_file(const int64_t length); // 截断
			int seek_file(const int64_t offset); // 查看当前位置
			
			int get_fd() const{
				return fd_;
			}
			
		
		protected:
			int fd_;
			int open_flags_;
			char* file_name_;
			static const mode_t OPEN_MODE = 0644; //权限：user/group/other:110|100|100:rwx|rwx|rwx
			static const int MAX_DISK_TIMES = 5; // 最大磁盘读取次数 永久失败or暂时失败
			
			int check_file();
		};
		
		
		
	}
}




#endif //_FILE_OP_H_