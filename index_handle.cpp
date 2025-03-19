#include "index_handle.h"
#include "common.h"
#include <sstream>



namespace nmsp_tfs{
	namespace nmsp_large_file{
		IndexHandle::IndexHandle(const std::string& base_path,const uint32_t main_block_id){
			// create file_op_handle object
			std::stringstream tmp_stream;
			// base_path: /root/zerouser   /index/1
			tmp_stream << base_path <<INDEX_DIR_PREFIX << main_block_id;
			
			std::string index_path;
			tmp_stream >> index_path;
			
			file_op_ = new MMapFileOperation(index_path,O_RDWR|O_LARGEFILE|O_CREAT);
			is_load_ = false;
		}
		
		IndexHandle::~IndexHandle(){
			if(file_op_){
				delete file_op_;
				file_op_ = NULL;
			}
		}
		
		int IndexHandle::create(const uint32_t logic_block_id, const int32_t bucket_size, const MMapOption mmap_option){
			int ret = TFS_SUCCESS;
			
			if(debug) printf("create index, block id:%u, bucket size: %d, max_mmap_size: %d, first_mmap_size: %d, per_mmap_size: %d\n", logic_block_id,bucket_size,mmap_option.max_mmap_size_,mmap_option.first_mmap_size_,mmap_option.per_mmap_size_);
			
			if(is_load_){
				return EXIT_INDEX_ALREADY_LOADED_ERROR;
			}
			
			int64_t file_size = file_op_->get_file_size();
			
			printf("file size = %d\n", file_size);
			
			if(file_size<0){

				return TFS_ERROR;
			}
			else if(file_size==0){

				IndexHeader i_header;

				i_header.block_info_.block_id_ = logic_block_id;
				i_header.block_info_.seq_no_ = 1;
				i_header.bucket_size_ = bucket_size;
				
				i_header.index_file_size_ = sizeof(IndexHeader) + bucket_size*sizeof(int32_t);
				
				// index header + total buckets
				char* init_data = new char[i_header.index_file_size_];
				memcpy(init_data, &i_header, sizeof(IndexHeader));
				memset(init_data+sizeof(IndexHeader), 0, i_header.index_file_size_-sizeof(IndexHeader));
				
				printf("init_data............\n");

				
				// write index header and buckets into index file
				ret = file_op_->pwrite_file(init_data,i_header.index_file_size_, 0);
				
				delete[] init_data;
				init_data = NULL;
				
				if(ret!=TFS_SUCCESS){
					return ret;
				}
				ret = file_op_->flush_file();
				
				if(ret!=TFS_SUCCESS){
					return ret;
				}
				
				printf("end file_size............%d\n",file_size);

			}
			else{ // file size > 0, index already exist
				printf("1 EXIT_META_UNEXPECT_FOUND_ERROR\n");
				return EXIT_META_UNEXPECT_FOUND_ERROR;
			}
			
			ret = file_op_->mmap_file(mmap_option);
			if(ret!=TFS_SUCCESS){
				return ret;
			}
			
			is_load_ = true;
			
			if(debug) printf("init blockid: %u index successful. data file size: %d, index file size: %d, bucket size: %d, free head offset: %d, seqno:%u, filecount:%d, del_size: %d, del_file_count: %d version: %u\n", logic_block_id,index_header()->data_file_offset_,index_header()->index_file_size_,index_header()->bucket_size_,index_header()->free_head_offset_,block_info()->seq_no_,block_info()->file_count_,block_info()->del_size_,block_info()->del_file_count_, block_info()->version_);
			
			return TFS_SUCCESS;	
		}
		
		
		int IndexHandle::load(const uint32_t logic_block_id, const int32_t _bucket_size, const MMapOption mmap_option){
			if(is_load_){
				return nmsp_large_file::EXIT_INDEX_ALREADY_LOADED_ERROR;
			}
			int ret = TFS_SUCCESS;
			int64_t file_size = file_op_->get_file_size();
			if(file_size<0){
				return TFS_ERROR;
			}
			else if(file_size==0) // empty file
			{
				return EXIT_INDEX_CORRUPT_ERROR;
			}
			
			// file_size > 0 映射到内存
			MMapOption tmp_mmap_option = mmap_option;
			
			if(file_size > tmp_mmap_option.first_mmap_size_ && file_size <= tmp_mmap_option.max_mmap_size_){
				tmp_mmap_option.first_mmap_size_ = file_size;
			}
			
			ret = file_op_->mmap_file(tmp_mmap_option);
			
			if(ret!=TFS_SUCCESS){
				return ret;
			}
			
			if(0==bucket_size()||0==block_info()->block_id_){
				fprintf(stderr,"index corrupt error, block id: %u, bucket size: %d\n", block_info()->block_id_,bucket_size());
				return EXIT_INDEX_CORRUPT_ERROR;
			}
			
			// check file size
			int32_t index_file_size = sizeof(IndexHeader) + bucket_size()*sizeof(int32_t);
			
			if(file_size < index_file_size){
				fprintf(stderr,"index corrupt error, block id: %u, bucket size: %d, file size: %d, index file size: %d\n", block_info()->block_id_,bucket_size(), file_size, index_file_size);
				return EXIT_INDEX_CORRUPT_ERROR;
			}
			
			// check block id size
			if(logic_block_id != block_info()->block_id_){
				fprintf(stderr,"block id conflict, block id: %u, index blockid: %u\n", logic_block_id, block_info()->block_id_);
				return EXIT_BLOCKID_CONFLICT_ERROR;
			}
		    
			// check bucket size
			if(_bucket_size != bucket_size()){
				fprintf(stderr,"bucket size error, old bucket size: %d, old bucket size: %d\n", bucket_size(), _bucket_size);
				return EXIT_BUCKET_CONFIGURE_ERROR;
			}
			
			is_load_ = true;
			
			if(debug) printf("load blockid: %u index successful. data file size: %d, index file size: %d, bucket size: %d, free head offset: %d, seqno:%u, filecount:%d,del_size: %d, del_file_count: %d version: %u\n", 
			logic_block_id,index_header()->data_file_offset_,index_header()->index_file_size_,index_header()->bucket_size_,index_header()->free_head_offset_,block_info()->seq_no_,block_info()->file_count_,block_info()->del_size_,block_info()->del_file_count_,block_info()->version_);	
			
			return TFS_SUCCESS; 
		}
		
		
		int IndexHandle::remove(const uint32_t logic_block_id){
			
			if(is_load_){
				if(logic_block_id != block_info()->block_id_){
					fprintf(stderr,"blockid conflict! blockid: %d, index blockid: %d\n", logic_block_id, block_info()->block_id_);
					return EXIT_BLOCKID_CONFLICT_ERROR;
				}
			}
			// ?
			int ret = file_op_->munmap_file();
			if(ret!=TFS_SUCCESS){
				return ret;
			}
			
			ret = file_op_->unlink_file();
			
			return ret;
		}
		
		int IndexHandle::flush(){
			int ret = file_op_->flush_file();
			if(ret!=TFS_SUCCESS){
				fprintf(stderr,"Index flush failed! ret: %d, desc: %s\n", ret, strerror(errno));
			}
			return ret;
		}
		
		int IndexHandle::update_block_info(const OperType oper_type, const uint32_t modify_size){
			if(block_info()->block_id_==0){
				return EXIT_BLOCKID_ZERO_ERROR;
			}
			
			if(oper_type==C_OPER_INSERT){
				++block_info()->version_;
				++block_info()->file_count_;
				++block_info()->seq_no_;
				block_info()->size_ += modify_size;
			}
			else if(oper_type==C_OPER_DELETE){
				++block_info()->version_;
				--block_info()->file_count_;
				block_info()->size_ -= modify_size;
				++block_info()->del_file_count_;
				block_info()->del_size_ += modify_size;
			}
			
			if(debug) printf("update block info. blockid: %u, version: %u, size: %d. seq_no: %u, filecount:%d,del_size: %d, del_file_count: %d, operType:%d \n",block_info()->block_id_,block_info()->version_, block_info()->size_, block_info()->seq_no_, block_info()->file_count_,block_info()->del_size_,block_info()->del_file_count_, oper_type);	
			
			return TFS_SUCCESS;
		}
		
		
		int32_t IndexHandle::write_segment_meta(const uint64_t key, MetaInfo& meta){
			int32_t current_offset = 0;
			int32_t pre_offset = 0;
			
			//1. key是否存在
			// 从文件哈希表中查找key hash_find(key, current_offset, pre_offset);
			int32_t ret = hash_find(key,current_offset, pre_offset);
			
			if(ret == TFS_SUCCESS){
				return EXIT_META_UNEXPECT_FOUND_ERROR;
			}
			else if(ret!=EXIT_META_NOT_FOUND_ERROR){
				return ret;
			}
			//2. 不存在就写入meta到文件哈希表中 hash_insert(slot, pre_offset, meta);
			ret = hash_insert(key, pre_offset,meta);
			
			return ret;
		}
		
		
		int32_t IndexHandle::read_segment_meta(const uint64_t key, MetaInfo& meta){
			int32_t current_offset = 0;
			int32_t pre_offset = 0;
			
			// 1. key是否存在
			int32_t ret = hash_find(key,current_offset, pre_offset);
			if(ret == TFS_SUCCESS){// 存在
				ret = file_op_->pread_file(reinterpret_cast<char*>(&meta),sizeof(MetaInfo),current_offset);
				return ret;
			}
			else{
				return ret;
			}			
		}
		
		int32_t IndexHandle::delete_segment_meta(const uint64_t key){
			int32_t current_offset = 0;
			int32_t pre_offset = 0;
			
			// 1. key是否存在
			int32_t ret = hash_find(key,current_offset, pre_offset);
			if(ret != TFS_SUCCESS){// 不存在
				return ret;
			}
			// 存在，就删除
			
			MetaInfo meta;
			ret = file_op_->pread_file(reinterpret_cast<char*>(&meta),sizeof(MetaInfo),current_offset);
			if(ret != TFS_SUCCESS){// 没读到
				return ret;
			}
			
			int32_t next_pos = meta.get_next_meta_offset();
			
			//case 1: 没有前置节点
			if(pre_offset==0){
				int32_t slot = static_cast<uint32_t>(key) % bucket_size();
				bucket_slot()[slot] = next_pos;
			}
			
			//case 2: 有前置节点
			else{
				MetaInfo pre_meta;
				ret = file_op_->pread_file(reinterpret_cast<char*>(&pre_meta),sizeof(MetaInfo),pre_offset);
				if(ret != TFS_SUCCESS){// 没读到
					return ret;
				}
				
				pre_meta.set_next_meta_offset(next_pos);
				
				ret = file_op_->pwrite_file(reinterpret_cast<char*>(&pre_meta),sizeof(MetaInfo),pre_offset);
				if(ret != TFS_SUCCESS){// 没读到
					return ret;
				}	
			}
			
			// 被删的节点要放在 可重用的节点链表 中
			
			meta.set_next_meta_offset(get_free_head_offset()); // 头插法
			ret = file_op_->pwrite_file(reinterpret_cast<char*>(&meta),sizeof(MetaInfo),current_offset);
			if(ret!=TFS_SUCCESS){
				return ret;
			}
			
			index_header()->free_head_offset_ = current_offset;
			
			if(debug) printf("delete segment meta, current_offset: %d\n", current_offset);

			update_block_info(C_OPER_DELETE, meta.get_size());

			return TFS_SUCCESS;
			
		}
		
		
		int32_t IndexHandle::hash_find(const uint64_t key, int32_t& current_offset, int32_t& pre_offset){
			int ret = TFS_SUCCESS;
			MetaInfo meta_info;
			
			current_offset = 0;
			pre_offset = 0;
			//1. 确定key对应的桶slot
			int32_t slot = static_cast<uint32_t>(key) % bucket_size();
			
			//2. 读取桶首节点存储的节点的偏移，如果偏移为0(即为空)，则返回EXIT_META_NOT_FOUND_ERROR
			//3. 根据偏移读取存储的meteinfo
			//4. metainfo与key比较不相等则依次向下个节点比较,直至为空或找到
			
			int32_t pos = bucket_slot()[slot];
			
			if(pos!=0){
				ret = file_op_->pread_file(reinterpret_cast<char*>(&meta_info),sizeof(MetaInfo), pos);
				if(ret!=TFS_SUCCESS){
					return ret;
				}
				
				if(hash_compare(key, meta_info.get_key())){
					current_offset = pos;
					return TFS_SUCCESS;
				}
				pre_offset = pos;
				pos = meta_info.get_next_meta_offset();
				
			}

			return EXIT_META_NOT_FOUND_ERROR;
		}
		
		int32_t IndexHandle::hash_insert(const uint64_t key, int32_t pre_offset,  MetaInfo& meta){
			int ret = TFS_SUCCESS;
			MetaInfo tmp_meta;
			int32_t current_offset = 0;
			//1. 
			int32_t slot = static_cast<uint32_t>(key) % bucket_size();
			
			//2. 确定meta存在文件中的偏移
			// 是否有可重用节点
			if(get_free_head_offset()!=0){
				ret = file_op_->pread_file(reinterpret_cast<char*>(&tmp_meta), sizeof(MetaInfo), get_free_head_offset());
				if(ret!=TFS_SUCCESS){
					return ret;
				}
				current_offset = index_header()->free_head_offset_;
				
				if(debug) printf("reuse metainfo, current_offset: %d\n", current_offset);
				
				index_header()->free_head_offset_ = tmp_meta.get_next_meta_offset();
			}
			else{
				current_offset = index_header()->index_file_size_;
				index_header()->index_file_size_ += sizeof(MetaInfo);
			}
			
			//3. 将meta写入索引文件中
			meta.set_next_meta_offset(0);
			ret = file_op_->pwrite_file(reinterpret_cast<char*>(&meta), sizeof(MetaInfo), current_offset);
			if(ret!=TFS_SUCCESS){
				index_header()->index_file_size_ -= sizeof(MetaInfo);
				return ret;
			}
			
			//4. pre指向meta
			if(pre_offset!=0){
				
				ret = file_op_->pread_file(reinterpret_cast<char*>(&tmp_meta), sizeof(MetaInfo), pre_offset);
				if(ret!=TFS_SUCCESS){
					index_header()->index_file_size_ -= sizeof(MetaInfo);
					return ret;
				}
				
				tmp_meta.set_next_meta_offset(current_offset);
				
				ret = file_op_->pwrite_file(reinterpret_cast<const char*>(&tmp_meta), sizeof(MetaInfo), pre_offset);
				if(ret!=TFS_SUCCESS){
					index_header()->index_file_size_ -= sizeof(MetaInfo);
					return ret;
				}
			}
			else{// 要插入的节点为此桶槽的首个节点
				bucket_slot()[slot] = current_offset;
				
			}
			
			return TFS_SUCCESS;
		}
		
	}
}