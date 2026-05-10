aux_source_directory(. SRC_LIST)
#指定生成的可执行文件
add_executable(chatclient ${SRC_LIST} ${REDIS_LIST})
#生成该可执行文件所依赖的链接库文件
#link_directories(/usr/local/lib)
link_directories(/usr/lib64/mysql)
target_link_libraries(chatclient muduo_net muduo_base pthread  /usr/lib64/mysql/libmysqlclient.so.21.2.43)