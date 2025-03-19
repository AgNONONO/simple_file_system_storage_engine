all:
	g++ block_init_test.cpp index_handle.cpp file_op.cpp mmap_file_op.cpp mmap_file.cpp -o blockInitTest -std=c++11
	g++ block_state_test.cpp index_handle.cpp file_op.cpp mmap_file_op.cpp mmap_file.cpp -o blockStateTest -std=c++11
	g++ block_delete_test.cpp index_handle.cpp file_op.cpp mmap_file_op.cpp mmap_file.cpp -o blockDeleteTest -std=c++11
	g++ block_write_test.cpp index_handle.cpp file_op.cpp mmap_file_op.cpp mmap_file.cpp -o blockWriteTest -std=c++11
	g++ block_read_test.cpp index_handle.cpp file_op.cpp mmap_file_op.cpp mmap_file.cpp -o blockReadTest -std=c++11

clean:
	rm blockInitTest blockStateTest blockDeleteTest blockWriteTest blockReadTest
