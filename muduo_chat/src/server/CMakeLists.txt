#定义了一个SRC_LIST变量，包含了该目录下所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(../hiredis REDIS_LIST)
#指定生成的可执行文件
add_executable(chatserver ${SRC_LIST} ${REDIS_LIST})
#生成该可执行文件所依赖的链接库文件
#link_directories(/usr/local/lib)
link_directories(/usr/lib64/mysql)
link_directories(/usr/local/lib)
target_link_libraries(chatserver muduo_net muduo_base pthread  /usr/lib64/mysql/libmysqlclient.so.21.2.43 /usr/local/lib/libhiredis.so.1.3.0)