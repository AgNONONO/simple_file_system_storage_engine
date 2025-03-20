#ifndef _FILE_OP_H_
#define _FILE_OP_H_

#include "common.h"


namespace nmsp_fsse{
	namespace nmsp_large_file{
		
		class FileOperation{
		public:
			FileOperation(const std::string &file_name, const int open_flags = O_RDWR|O_LARGEFILE);
			~FileOperation();
			
			int open_file();     	// 打开文件
			void close_file();		// 关闭文件
			
			virtual int flush_file(); // 将内存数据写入磁盘
			
			int unlink_file(); // 逻辑删除文件
			
			virtual int pread_file(char* buf, const int32_t nbytes, const int64_t offset); // 
			virtual int pwrite_file(const char* buf, const int32_t nbytes, const int64_t offset); // seek来看偏移
			int write_file(const char* buf, const int32_t nbytes); // 从当前位置写入
			
			int64_t get_file_size();	// 获取文件描述符对应文件的大小
			
			int ftruncate_file(const int64_t length); // 截断
			int seek_file(const int64_t offset); // 将文件指针移动到指定偏移位置（相对于文件头）
			
			int get_fd() const{ // 获取文件描述符
				return fd_;
			}
			
		
		protected:
			int fd_;           // 要操作的文件的文件描述符
			int open_flags_;   // 文件打开方式
			char* file_name_; // 要操作的文件
			static const mode_t OPEN_MODE = 0644; //权限：user/group/other:110|100|100:rwx|rwx|rwx
			static const int MAX_DISK_TIMES = 5; // 最大磁盘读取次数 永久失败or暂时失败
			
			int check_file(); // 核验是否打开了文件 fd_
		};
		
		
		
	}
}




#endif //_FILE_OP_H_