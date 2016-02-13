# gaintConcurrent 
# 大并发服务器
	
	本项目使用cmake来动态编译源代码
	Ubuntu下使用`sudo apt-get install cmake`安装cmake
	
	大致使用流程:
		创建一个CMakeLists.txt文件,根据cmake的语法规则写文件
		然后执行`cmake .`, cmake是CMakeLists.txt所在的目录位置
		此时会生成一个build文件,在里面会自动生成一个Makefile
		执行这个Makefile,文件会生成在build/bin这个文件夹中
		
	不过在本项目中,每个CMakeLists.txt的文件夹中有一个build.sh的脚本
	这个脚本会自动执行makefile,并在项目根目录中的build/bin中生成程序

##poll
    
    每次调用poll函数的时候,都需要把监听套接字与已接入的套接字所感兴趣的事件数组拷贝到内核空间
    所以效率会比较低
    
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
    	4. 提前准备一个空闲的文件描述符
    	5. 通过应用层协议判断,关闭掉不必要的文件描述符(个人意见)
    
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
    			
##epoll

	- LT 水平触发模式 和poll运行方式大致相同
		在高电平的时候,触发事件,	
	
	- ET 边缘触发模式
		在低电平到高电平之间的时候,触发事件
	
		由于使用的是边缘触发模式,在一次触发读事件之后,如果没有把内容读空,
		如果读到一半,就重新监听的话,就算还有数据也不会再次触发读事件了,
		只有下一次有新的内容发送过来的时候,才会重新触发读事件,
		所以使用ET模式的话,一定要把非阻塞文件描述符读到EAGAIN错误,
		否则就会导致错误发生
		
		eg: 
			listenfd EPOLLIN事件到来
			connfd = accept(...);
			关注connfd的EPOLLIN事件与EPOLLOUT事件
			
			处理活跃套接字
			connfd EPOLLIN事件到来
			read(connfd, ...);
			read直到返回EAGAIN错误
			ret = write(connfd, buf, 10000);
			if (ret < 10000)
				将未发送完的数据添加到应用层缓冲区OutBuffer
			connfd EPOLLOUT事件到来
			取出应用层缓冲区中的数据发送 write(connfd, ...);
			直到应用层缓冲区数据发完,或者发送返回EAGAIN
		
##面向对象的线程类

	通过继承对象,重写成员函数来使用线程类

	成员函数的第一个参数是this,如果回调,是thiscall约定,
	而一般的C语言的回调是普通的函数约定,所以一般的成员函数,作为回调函数使用 
	但是静态成员函数可以.
	
	虚函数具有回调的功能
	线程对象的生命周期与线程的生命周期是不一样
	
	线程执行完毕,线程对象能够自动销毁
		方法就是:首先对象要动态创建,才可以动态销毁
				设置一个状态字,当Run()函数运行完之后,判断状态字
				是真就delete销毁对象,所以要有一个成员函数来单独设置销毁的状态字

##基于对象的线程类			

	通过直接调用线程类,传入回调函数来执行任务 
	可以通过boost的bind函数,把成员函数传入

	- boost
		函数适配器:boost_test,把memberFunc(this, double, int, int)四个参数的成员函数,
			适配成一个void (int)的一个参数的函数, _1是参数占位符
	- explicit
		修饰的函数不会执行隐式转换
	
##muduo库

###安装
	sudo apt-get install cmake
	sudo apt-get install libboost-dev
	
	muduo的源码可以直接从github上下载
	https://github.com/chenshuo/muduo.git
	
	使用source insight查看代码
	
	安装TabSiPlus分页插件

### muduo_base库源码分析

#### Timestamp类封装,时间戳类		
	继承的基类
		muduo::copyable 空基类,标示类,值类型,
				凡是继承自这个基类的类都是可以拷贝的
		boost::less_than_complate<Timestamp> 是boost <base/types.h>里面的类模板
				一旦继承这个类,就必须实现<运算符重载,然会他会自动实现>,<=,>=
	
	类unix系统的时间起始点是1970-01-01 00:00:00
	
	BOOST_STATIC_ASSERT:编译时断言  <boost/static_assert.hpp>
		BOOST_STATOC_ASSERT(sizeof(int) == sizeof(short));
		上面这条语句在编译是就会判断,导致编译出错
			error: invalid application of ‘sizeof’ to incomplete type ‘boost::STATIC_ASSERTION_FAILURE<false>’
			BOOST_STATIC_ASSERT(sizeof(int) == sizeof(short));
	
	assert是运行时断言
	
	使用PRld64
		int64_t 表示64为整数,在32位系统中打印是printf("%lld", value);
							 在64位系统中打印是printf("%ld", value);
							 
		跨平台的做法是:
			#define _STDC_FORMAT_MACROS
			#include <inttypes.h>
			#undef _STDC_FORMAT_MACROS
			
			printf("%" PRId64 "\n", value);
	
	Timestamp实现及测试

	这个是这个类的测试用例
	muduo/muduo/base/tests/Timestamp_unittest.cc
	
#### 原子性操作
	gcc提供了一系列原子性操作
	//原子自增操作
	type __sync_fetch_and_add (type *ptr, type value);
	
	//原子比较和交换(设置)操作
	type __sync_val_compare_and_swap(type *ptr, type oldval type newval)
	bool __sync_bool_compare_and_Swap(type *ptr, type oldval type newval)
	
	//原子赋值操作
	type __sync_lock_test_and_set(type *ptr, type value)
	使用这些原子操作,编译时需要加 -march=cpu-type
	
	参考博文:http://www.cnblogs.com/FrankTan/archive/2010/12/11/1903377.html
	
	无锁队列的实现:http://coolshell.cn/articles/8239.html

#### AtomicIntegerT类封装, 原子性整数操作
	实现了自增,自减,前自增,和后自增,自减相同

	- 继承的基类
		boost::noncopyable  不可拷贝的类, 大体原理就是把=运算符做成私有的

	- return value
		因为都是原子性操作,所以返回的时候不需要再找一个值来作为中间值,直接从函数返回即可

	- volatile的作用:作为指令关键字,确保本条指令不会因编译器的优化而省略,且要求每次直接读值
				简单地说就是防止编译器对代码进行优化
			当要求使用volatile声明变量的时候,系统总是重新从它所在的内存读取数据,
			而不是使用保存在寄存器中的备份.即使它前面的指令刚刚从该处读取过数据.
			而且读取的数据立刻被保存.

####Exception类实现

	主要作用是抛异常
		使用这个类抛异常,可以显示出错时函数运行状态,是怎样的嵌套关系
		定位问题点
	
	- backtrace, 栈回溯,保存各个栈帧的地址
	
	- backtrace_symbols, 根据地址,转成相应的函数符号
	
	- abi::__cxa_demangle

####线程封装

	- 线程标识符
		pthread_self
		gettid()可以得到tid,但glibc并没有实现该函数,只能通过Linux的系统调用syscall来获取
			return syscall(SYS_gettid)
			
	- __thread, gcc内置的线程局部存储设施
		__thread只能修饰POD类型, 线程销毁,该变量也销毁
		POD类型(plain old data), 与C兼容的原始数据,
		例如,结构和整型等C语言中的类型是POD类型,
		但带有用户定义的构造函数或虚函数的类则不是
		
	- boost::is_same
		判定两种类型是否相同
	
	- pthread_atfork
		主要的作用就是清理fork前后,锁的状态
	
		int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));  
		
		这一函数的作用是为fork安装三个帮助清理锁的函数。其中：
		    prepare函数由父进程在fork创建子进程之前调用，这个fork处理程序的任务是获取父进程定义的所有锁；
		    parent函数在fork创建子进程后，但在fork返回之前在父进程环境中调用的，其任务是对prepare获取的所有锁进行解锁；
		    child函数是在fork返回前在子进程环境中调用的，和parent函数一样，child函数也必须释放prepare处理函数中的所有的锁。
	 	
	 	重点来了，看似这里会出现加锁一次，解锁两次的情况。其实不然，因为fork后对锁进行操作时，
	 	子进程和父进程通过写时复制已经不对相关的地址空间进行共享了，所以，此时对于父进程，
	 	其释放原有自己在prepare中获取的锁，而子进程则释放从父进程处继承来的相关锁。两个并不冲突。

	创建一个线程类的基本过程
		首先创建一个Thread类,把回调函数传进去, 
		然后就是Thread::start()
		此时start()函数会创建一个ThreadData类,并把回调函数传进去,
		然后调用pthread_create运行detail::startThread,参数就传刚创建的ThreadData这个对象
		然后在detail::startThread里面,会调用ThreadData::runInThread 函数
		在ThreadData::runInThread这个函数中调用我们想让他调用的那个回调

### muduo_net库源码分析
	
    	
    	
    	
    	