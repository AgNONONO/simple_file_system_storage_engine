#ifndef _COMMON_H_
#define _COMMON_H_

#include <iostream>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <assert.h>

/* 公共部分 */

namespace nmsp_fsse{
	namespace nmsp_large_file{
		
		static int debug = 1;
		
		const int32_t FSSE_SUCCESS = 0;
		const int32_t FSSE_ERROR = -1;
		
		const int32_t EXIT_DISK_OPER_INCOMPLETE = -8012;
		// 读或写的长度小于所请求的
		
		const int32_t EXIT_INDEX_ALREADY_LOADED_ERROR = -8013; // index is loaded when create or load
		const int32_t EXIT_META_UNEXPECT_FOUND_ERROR = -8014; // index is loaded when create or load
		const int32_t EXIT_INDEX_CORRUPT_ERROR = -8015; // index is corrupt
		const int32_t EXIT_BLOCKID_CONFLICT_ERROR = -8016; // index is corrupt
		const int32_t EXIT_BUCKET_CONFIGURE_ERROR = -8017; // index is corrupt
		const int32_t EXIT_META_NOT_FOUND_ERROR = -8018; // index is corrupt
		const int32_t EXIT_BLOCKID_ZERO_ERROR = -8019;
		
		static const std::string MAINBLOCK_DIR_PREFIX="/mainblock/";
		static const std::string INDEX_DIR_PREFIX="/index/";
		static const mode_t DIR_MODE = 0755;
		
		enum OperType{
			C_OPER_INSERT = 1,
			C_OPER_DELETE
			
		};
		
		// 记录内存映射时的可选项
		struct MMapOption{
			int32_t max_mmap_size_;     // 最大映射大小
			int32_t first_mmap_size_;   // 首次映射大小
			int32_t per_mmap_size_;		// 追加步长
		};
		
		struct BlockInfo{
			uint32_t block_id_;
			int32_t version_;
			int32_t file_count_;
			int32_t size_;
			int32_t del_file_count_;
			int32_t del_size_;
			uint32_t seq_no_;
			
			BlockInfo(){
				// 都清零
				memset(this,0,sizeof(BlockInfo));
			}
			
		    // 重载==
			inline bool operator==(const BlockInfo& rhs)const{
				return block_id_==rhs.block_id_ && version_==rhs.version_ && file_count_==rhs.file_count_ && size_==rhs.size_ && del_file_count_==rhs.del_file_count_ && del_size_==rhs.del_size_ && seq_no_ == rhs.seq_no_;
				
			}
			
		};
		
		
		
		struct MetaInfo{
		public:
			MetaInfo(){
				init();
			}
			
			MetaInfo(const uint64_t file_id, const int32_t inner_offset, const int32_t file_size, const int32_t next_meta_offset){
				fileid_ = file_id;
				location_.inner_offset_ = inner_offset;
				location_.size_ = file_size;
				next_meta_offset_ = next_meta_offset;
			}
			
			MetaInfo(const MetaInfo& meta_info){
				memcpy(this, &meta_info,sizeof(MetaInfo));
			}
			
			// 重载赋值运算符
			MetaInfo& operator=(const MetaInfo& meta_info){
				if(this==&meta_info){
					return *this;
				}
				fileid_ = meta_info.fileid_;
				location_.inner_offset_ = meta_info.location_.inner_offset_;
				location_.size_ = meta_info.location_.size_;
				next_meta_offset_ = meta_info.next_meta_offset_;
				
				return *this;
			}
			
			MetaInfo& clone(const MetaInfo& meta_info){
				assert(this!=&meta_info);
				
				fileid_ = meta_info.fileid_;
				location_.inner_offset_ = meta_info.location_.inner_offset_;
				location_.size_ = meta_info.location_.size_;
				next_meta_offset_ = meta_info.next_meta_offset_;	
				
				return *this;
			}
			// 重载比较
			bool operator==(const MetaInfo& rhs) const {
				
				return  fileid_ == rhs.fileid_ 
				&&location_.inner_offset_ == rhs.location_.inner_offset_
				&&location_.size_ == rhs.location_.size_
				&&next_meta_offset_ == rhs.next_meta_offset_;
			}
			
			// 经典接口
			uint64_t get_key()const{
				return fileid_;
			}
			void set_key(const uint64_t key){
				fileid_ = key;
			}
			uint64_t get_file_id()const{
				return fileid_;
			}
			void set_file_id(const uint64_t file_id){
				fileid_ = file_id;
			}
			int32_t get_offset(){
				return location_.inner_offset_;
			}
			void set_offset(const int32_t offset){
				location_.inner_offset_ = offset;
			}
			uint32_t get_size(){
				return location_.size_;
			}
			void set_size(const int32_t size){
				location_.size_ = size;
			}
			int32_t get_next_meta_offset(){
				return next_meta_offset_;
			}
			void set_next_meta_offset(int32_t next_meta_offset){
				next_meta_offset_ = next_meta_offset;
			}
		
		
		private:
			uint64_t fileid_;
			
			struct{
				int32_t inner_offset_;
				int32_t size_;
			}location_;
			
			int32_t next_meta_offset_;
			
			// 初始化
		private:
			void init(){
				fileid_ = 0;
				location_.inner_offset_ = 0;
				location_.size_ = 0;
				next_meta_offset_ = 0;
			}
		};
	}
}
#endif // _COMMON_H_
