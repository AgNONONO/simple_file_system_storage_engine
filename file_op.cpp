#include "file_op.h"
#include "common.h"

namespace nmsp_fsse{
	namespace nmsp_large_file{
		
		FileOperation::FileOperation(const std::string &file_name, const int open_flags):
		fd_(-1),open_flags_(open_flags){
			// 函数主要是拷贝字符串s的一个副本，由函数返回值返回，这个副本有自己的内存空间，和s没有关联。
			// strdup函数
			// 复制一个字符串，使用完后，要使用delete函数删除在函数中动态申请的内存
			file_name_ = strdup(file_name.c_str());
		}
		FileOperation::~FileOperation(){
			if(fd_ > 0){
			// 前面加上冒号，表示要调用的是全局的，而不是当前作用域的
				::close(fd_);
			}
			if(file_name_!=NULL){
				free(file_name_);
				file_name_ = NULL;
			}
		}
		
		int FileOperation::open_file(){
			if(fd_>0){
				::close(fd_);
				fd_ = -1;
			}
			
			fd_ = ::open(file_name_, open_flags_,OPEN_MODE);
			if(fd_ < 0){
				return -errno;
			}
			return fd_;
		}
		
		void FileOperation::close_file(){
			if(fd_<0){
				return;
			}
			::close(fd_);
			fd_ = -1;
		}
		
		int64_t FileOperation::get_file_size(){
			int fd = check_file();
			if(fd < 0){
				return -1;
			}
			
			// 不打开文件，但也可以获得其大小
			struct stat statBuf;
			if(fstat(fd, &statBuf)!=0){
				return -1;
			}
			
			return statBuf.st_size;
			
		}
		
		int FileOperation::check_file(){
			if(fd_ < 0){
				fd_ = open_file();
			}
			return fd_;
		}
		
		int FileOperation::ftruncate_file(const int64_t length){
			int fd = check_file();
			
			if(fd < 0){
				return fd;
			}
			
			return ftruncate(fd, length); // 截断到指定的长度，可长可短
			
		}
		
		int FileOperation::seek_file(const int64_t offset){
			int fd = check_file();
			if(fd < 0){
				return fd;
			}
			// SEEK_SET: 文件头 SEEK_CUR: 文件当前位置 SEEK_END: 文件尾后
			return lseek(fd, offset, SEEK_SET);
		}
		
		int FileOperation::flush_file(){
			// O_SYNC 同步更新，不需要手动flush
			if(open_flags_ & O_SYNC){
				return 0;
			}
			
			int fd = check_file();
			if(fd < 0){
				return fd;
			}
			// fsync: 将文件缓冲区数据写回磁盘文件
			return fsync(fd);
		}
		
		int FileOperation::unlink_file(){
			int fd = check_file();
			if(fd < 0){
				return fd;
			}
			
			close_file();
			
			return ::unlink(file_name_);
		}
		
		// 以下 优秀重点实现
		
		int FileOperation::pread_file(char* buf, const int32_t nbytes, const int64_t offset){
			// pread()类似于 lseek+read，但不完全相同
			// pread与pwrite不会影响文件指针位置 offset默认从 文件头 开始偏移
			// 大文件用64位偏移
			// 由于服务器繁忙，很可能一次都不全，故不能仅靠一次读写就返回成功与否
					
			// 多次读写，相应的改变 剩余字节数 剩余偏移量
			int32_t left = nbytes; // 剩余字节数
			int64_t read_offset = offset; // 剩余偏移量
			int32_t read_len = 0; // 已读取的长度
			char* p_tmp = buf; // 存放读取到的数据的位置在buf中
			
			int i = 0; // 注意到之前设置了最大读取次数为5
			
			while(left > 0){
				++i;
				
				if(i>=MAX_DISK_TIMES){
					break;
				}
				
				if(check_file() < 0){
					return -errno;
				}
				
				read_len = ::pread64(fd_, p_tmp, left, read_offset);
				//调用pread相当于顺序调用了lseek 和　read，这两个操作相当于一个捆绑的原子操作
				
				if(read_len < 0){
					// 保存出错原因
					read_len = -errno;
					// error是全局变量，其可能被其他线程改变
					if(-read_len == EINTR||-read_len == EAGAIN) 
					{
						// EINTR：临时中断 EAGAIN: 再试
						continue;
					}else if(-read_len == EBADF){
						// EBADF: 坏文件符
						fd_ = -1;
						return read_len;
					}
					else{
						return read_len;
					}
				}
				else if(read_len == 0){
					break;
				}
				
				left -= read_len;
				p_tmp += read_len;
				read_offset += read_len;	
			}
			
			if(left!=0){
				return EXIT_DISK_OPER_INCOMPLETE;
			}
			
			return FSSE_SUCCESS;
		}
		
		
		
		int FileOperation::pwrite_file(const char* buf, const int32_t nbytes, const int64_t offset){
			
			// 多次读写，相应的改变 剩余字节数 剩余偏移量
			int32_t left = nbytes; // 剩余字节数
			int64_t write_offset = offset; // 剩余偏移量
			int32_t write_len = 0; // 已读取的长度
			char* p_tmp = (char*)buf; // 存放读取到的数据的位置在buf中
			
			int i = 0; // 注意到之前设置了最大读取次数为5
			
			while(left > 0){
				++i;
				
				if(i>=MAX_DISK_TIMES){
					break;
				}
				
				if(check_file() < 0){
					return -errno;
				}
				
				write_len = ::pwrite64(fd_, p_tmp, left, write_offset);
				// 调用pwrite相当于顺序调用了lseek 和　write，这两个操作相当于一个捆绑的原子操作
				
				if(write_len < 0){
					// 保存出错原因
					write_len = -errno;
					// error是全局变量，其可能被其他线程改变
					if(-write_len == EINTR||-write_len == EAGAIN) 
					{
						// EINTR：临时中断 EAGAIN: 再试
						continue;
					}else if(-write_len == EBADF){
						// EBADF: 坏文件符
						fd_ = -1;
						continue;
					}
					else{
						return write_len;
					}
				}
				else if(write_len == 0){
					//
					break;
				}
				
				left -= write_len;
				p_tmp += write_len;
				write_offset += write_len;	
			}
			
			if(left!=0){
				return EXIT_DISK_OPER_INCOMPLETE;
			}
			
			return FSSE_SUCCESS;
		}
		

		
		int FileOperation::write_file(const char* buf, const int32_t nbytes){
			
			int32_t left = nbytes; // 剩余字节数
			int32_t write_len = 0; // 已写的长度
			char* p_tmp = (char*)buf; // 将buf中的数据写入file_op类成员变量 fd_ 对应的文件
			
			int i = 0; // 注意到之前设置了最大读取次数为5
			
			while(left > 0){
				++i;
				
				if(i>=MAX_DISK_TIMES){
					break;
				}
				
				if(check_file() < 0){
					return -errno;
				}
				
				write_len = ::write(fd_, p_tmp, left); // 文件指针会自动移动，故无需offset
				// write是不带缓冲区的函数，会直接写入磁盘
				
				if(write_len < 0){
					// 保存出错原因
					write_len = -errno;
					// error是全局变量，其可能被其他线程改变
					if(-write_len == EINTR||-write_len == EAGAIN) 
					{
						// EINTR：临时中断 EAGAIN: 再试
						continue;
					}else if(-write_len == EBADF){
						// EBADF: 坏文件符
						fd_ = -1;
						continue;
					}
					else{
						return write_len;
					}
				}
				
				left -= write_len;
				p_tmp += write_len;	
			}
			
			if(left!=0){
				return EXIT_DISK_OPER_INCOMPLETE;
			}
			
			return FSSE_SUCCESS;
		}
		
	}
}