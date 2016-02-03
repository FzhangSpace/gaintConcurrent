# gaintConcurrent 
# 大并发服务器

##poll
    
    - SIGPIPE信号
        如果客户端先关闭套接字close,而此时服务器调用一次write,服务器会接收一个RST segment(TCP传输层),
        如果服务器再次调用了write,这个时候就会产生SIGPIPE,SIGPIPE默认动作是关闭进程.
        因为大多数服务器都是7*24的,不能随便的就终止进程,所以上来就把他屏蔽掉
        
    - TIME_WAIT状态
        应该尽可能的在服务器中避免出现TIME_WAIT状态
        如果服务器端主动断开连接(先于client调用close),服务器就会进入TIME_WAIT,
        此时linux会保留一部分系统资源,超时释放,所以对于大并发服务器来说应该尽可能避免TIME_WAIT状态
        方法:就是通过协议设计,尽可能使client来主动断开链接,这样就把TIME_WAIT状态分散到大量的客户端.
            如果客户端不活跃了,一些客户端不断开连接,这样子也会占用服务端的链接资源,
            此时服务器端也要有个机制来踢掉不活跃链接
    
    - socket的选项
        SOCK_NONBLOCK:设置非阻塞套接字, I/O复用, 也可以使用fcntl函数来设置
        SOCK_CLOEXEC:这个参数是控制close-on-exec,执行时关闭这个选项的,当这个参数默认是0,
        			当被设置成1的时候,具体作用在于当开辟其他进程调用exec()族函数时,
        			在调用exec函数之前为exec族函数释放对应的文件描述符
        			
    - vertor表示数组首地址
    	pollfds.begin() 	----> 迭代器
    	*pollfds.begin()	----> 迭代器里面的元素
    	&*pollfds.begin() 	----> 元素的首地址
    	c++ 11的标准,动态数组的首地址可以是: pollfds.data();
    
    - POLLOUT事件触发条件
    	connfd的内核发送缓冲区不满(可以容纳发送的数据了)
    
    - errno == EMFILE
    	在linux下实现高并发服务器,默认的文件描述符会限制导致socket的EMFILE错误,
    	该错误被描述为"Too many open files"
    	解决方法
    	1. 调高进程文件描述符数目
    	2. 死等
    	3. 退出程序
    	4. 通过应用层协议判断,关闭掉不必要的文件描述符(个人意见)
    
    - 可能遇到的问题
    	1. 如果read之后的,缓冲区没有读取完毕,那如果此时重新调用poll函数,
    		那么会立即触发POLLIN事件, 如果发过来的数据包分包了,可能会遇到问题
    	   所以应该将读到的数据最佳到到connfd的应用层缓冲区的末尾,根据协议来判断粘包问题
    	2. write应答的时候数据量比较大,导致内核中的发送缓冲区满了,此时,因为fd是非阻塞的,会继续向下执行
    		会导致数据丢失
			所以应该将要发送的数据写到一个应用层缓冲区 	
    		大致的逻辑
    			`ret = write(connfd, buf, 10000);
    			if (ret < 10000) {
    				将未发送玩的数据添加到应用层缓冲区OutBuff
    				关注connfd的POLLOUT事件
    			} 
    			connfd POLLOUT事件到来
    			取出应用层缓冲区的数据发送write(connfd,......);
    			如果应用层缓冲区的数据发送完毕,取消关注POLLOUT事件`
    			
    	
    	
    	
    	
    	
    	
    	
