#include <jni.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <android/log.h>
#include <filesystem>
#include <sys/socket.h>
#include <sys/un.h>
//add for testing rsc
//end
#define TAG "dj.tang" // 这个是自定义的LOG的标识 #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__) // 定义LOGD类型
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__) // 定义LOGD类型

const char *PATH = "/data/data/com.example.ndkdemo2/my.sock";
int m_child;
const char* userId;
void child_do_work();

bool child_create_channel();

void child_listen_msg();

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ndkdemo2_Watcher_createWatcher(JNIEnv *env, jobject thiz,jstring userId_) {
    userId =  env->GetStringUTFChars(userId_,0);
    //创建进程
    pid_t pid = fork();
    LOGD("pid = :%d,currentID = :%d,parentId = :%d,userId = :%s",pid,getpid(),getppid(),userId);
    if(pid < 0){

    } else if(pid == 0) {
        //子进程  守护进程
        //LOGD("currentID = :%d,parentId = :%d",getppid());
        //LOGD("pid = :%d,currentID = :%d,parentId = :%d",pid,getpid(),getppid());
        child_do_work();

    }
    else{
        //父进程
        // LOGD("currentID = :%d,parentId = :%d",getppid());

    }
}

void child_do_work() {
   //开启socket,服务端
   if(child_create_channel()){
       child_listen_msg();
   }
}

void child_listen_msg() {
    fd_set rfds;
    struct timeval timeout = {3,0};
    while (1){
        FD_ZERO(&rfds); //清空集合
        FD_SET(m_child,&rfds); //将一个给定的文件描述符加入集合之中
        int r = select(m_child+1,&rfds,NULL,NULL,&timeout);
        if(r > 0){
            char pkg[256] = {0};
            //保证所读到信息是指定apk客户端
            if(FD_ISSET(m_child,&rfds)){ //检查集合中指定的文件描述符是否可以读写
                LOGD("服务端socket进入read状态...");
                int result = read(m_child,pkg, sizeof(pkg));
                LOGD("客户端 apk 死掉了 重启...%s",userId);
                //执行am命令
                //execlp("am","am","startservice","--user",userId,"com.example.ndkdemo2/com.example.ndkdemo2.ProcessService",(char*)NULL);
                execlp("am","am","start","--user","0","-a","android.intent.action.VIEW","-d","http://www.google.com.hk",(char*)NULL);
                break;
            }
        }

    }
}
/**
 * 服务端读取信息
 * 客户端
 */
/*void child_listen_msg() {
    fd_set fdSet;
    struct timeval timeval1{3,0};
    while(1){
        //清空内容
        FD_ZERO(&fdSet);
        FD_SET(m_child,&fdSet);
        //如果是两个客户端就在原来的基础上+1以此类推，最后一个参数是找到他的时间超过3秒就是超时
        //select会先执行，会找到m_child对应的文件如果找到就返回大于0的值，进程就会阻塞没找到就不会
        int r = select(m_child+1,&fdSet,NULL,NULL,&timeval1);
        if(r>0){
            //缓冲区
            char byte[256] = {0};
            //阻塞式函数
            LOGD("读取消息后 %d", r);
            read(m_child,byte, sizeof(byte));
            LOGD("在这里===%s", userId);
            //不在阻塞，开启服务
            //新进程与原进程有相同的PID。
            execlp("am","am","startservice", "--user", userId,
                   "com.example.ndkdemo2/com.example.ndkdemo2.ProcessService",
                   (char *)NULL);
            break;
        }
    }

}*/

/**
 * 创建服务端sockte
 * @return 1;
 */
bool child_create_channel() {
    //socket可以跨进程，文件端口读写  linux文件系统  ip+端口 实际上指明文件
    int listenfd = socket(AF_LOCAL,SOCK_STREAM,0);
    unlink(PATH);
    struct sockaddr_un addr;
    //清空刚刚建立的结构体，全部赋值为零
    memset(&addr,0, sizeof(sockaddr_un));
    addr.sun_family = AF_LOCAL;
    //    addr.sun_data = PATH; 不能够直接赋值,所以使用内存拷贝的方式赋值
    strcpy(addr.sun_path,PATH);
    //相当于绑定端口号
    if (bind(listenfd,(const sockaddr*)&addr, sizeof(sockaddr_un))<0){
        LOGD("绑定错误");
    }
    int connfd = 0;
    //能够同时连接5个客户端,最大为10
    listen(listenfd,5);
    //用死循环保证连接能成功
    while(1){
        //返回客户端地址 accept是阻塞式函数,返回有两种情况，一种成功，一种失败
        if ((connfd = accept(listenfd,NULL,NULL))<0){
            //有两种情况
            if (errno == EINTR){
                //成功的情况下continue继续往后的步骤
                continue;
            } else{
                LOGD("读取错误");
            }
        }
        m_child = connfd;
        LOGD("APK 父进程连接上了 %d", m_child);
        break;
    }
    return 1;

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ndkdemo2_Watcher_connectMonitor(JNIEnv *env, jobject thiz) {
    //客户端进程调用
    int socked;
    struct sockaddr_un addr;
    while(1){
        LOGD("客户端父进程开始连接");
        socked = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (socked < 0) {
            LOGD("连接失败");
            return;
        }
        memset(&addr, 0, sizeof(sockaddr_un));
        addr.sun_family = AF_LOCAL;
//    addr.sun_data = PATH; 不能够直接赋值
        strcpy(addr.sun_path, PATH);
        if(connect(socked, (const sockaddr *)(&addr), sizeof(sockaddr_un)) < 0){
            LOGD("连接失败");
            //如果连接失败了就关闭当前socked，休眠一秒重新开始连接
            close(socked);
            sleep(1);
            continue;
        }
        LOGD("连接成功");
        break;
    }
}