
#include "morn_util.h"

#if defined MORN_USE_SOCKET

#if defined(_WIN64)||defined(_WIN32)
#include <winsock2.h>
#define ADDR(S) (S).S_un.S_addr
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#define SOCKET int
#define closesocket close
#define ADDR(S) (S).s_addr
#define SOCKET_ERROR (-1)
#endif
 
#ifdef __GNUC__
#define strnicmp strncasecmp
#endif

void IPAddress(const char *ip,uint32_t *addr,uint32_t *port)
{
    if(ip==NULL) return;
    unsigned char *p = (unsigned char *)addr;
    if(strnicmp(ip,"localhost",9)==0) {p[0]=127,p[1]=0;p[2]=0;p[3]=1;ip=ip+9;}
    if(ip[0]==':') {*port=atoi(ip+1);return;}

    int a[4];
    sscanf(ip,"%d.%d.%d.%d:%d",a,a+1,a+2,a+3,port);
    p[0]=a[0];p[1]=a[1];p[2]=a[2];p[3]=a[3];
}

struct HandleUDP
{
    SOCKET udp;
    uint32_t addr;
    uint32_t port;
    struct sockaddr_in send_addr;
    struct sockaddr_in recive_addr;
    char addr_str[32];
    int wait_time;
};
void endUDP(struct HandleUDP *handle)
{
    closesocket(handle->udp);
    #if defined _WIN64
    WSACleanup();
    #endif
}

int UDPSetup(struct HandleUDP *handle)
{
    #if defined _WIN64
    WSADATA wsa;
    WORD version = MAKEWORD(2,2);
    if(WSAStartup(version, &wsa)) return MORN_FAIL;
    #endif
    
    handle->udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(handle->udp<0) return MORN_FAIL;

    return MORN_SUCCESS;
}

#define HandleUDPWrite HandleUDP
void endUDPWrite(struct HandleUDP *handle) {endUDP(handle);}
#define HASH_UDPWrite 0xc953a45
char *m_UDPWrite(MObject *obj,const char *address,void *data,int size)
{
    mException((data==NULL),EXIT,"invalid input size");
    if(size<0) size=strlen(data);
    
    MHandle *hdl=mHandle(obj,UDPWrite);
    struct HandleUDP *handle = hdl->handle;
    uint32_t addr=handle->addr;uint32_t port=handle->port;
    if(address!=NULL) IPAddress(address,&addr,&port);
    if((hdl->valid==0)||(handle->addr!=addr)||(handle->port!=port))
    {
        handle->addr = addr;handle->port=port;
        if(hdl->valid==0) {if(UDPSetup(handle)==MORN_FAIL) return 0;}

        handle->send_addr.sin_family = AF_INET;
        handle->send_addr.sin_port = htons(handle->port);
        ADDR(handle->send_addr.sin_addr) = handle->addr;
        // if(connect(handle->udp,(struct sockaddr*)&(handle->send_addr),sizeof(struct sockaddr))<0) return 0;

        hdl->valid=1;
    }
    
    sendto(handle->udp,data,size,0, (struct sockaddr *)&(handle->send_addr),sizeof(struct sockaddr_in));

    uint8_t *s=(uint8_t *)(&(ADDR(handle->send_addr.sin_addr)));
    sprintf(handle->addr_str,"%d.%d.%d.%d:%d",s[0],s[1],s[2],s[3],handle->port);
    
    return handle->addr_str;
}

#define HandleUDPRead HandleUDP
void endUDPRead(struct HandleUDP *handle) {endUDP(handle);}
#define HASH_UDPRead 0xf222ef4a
char *m_UDPRead(MObject *obj,const char *address,void *data,int *size)
{
    mException((size==NULL)||(data==NULL),EXIT,"invalid input size");

    MHandle *hdl=mHandle(obj,UDPRead);
    struct HandleUDP *handle = hdl->handle;
    uint32_t addr=htonl(INADDR_ANY);uint32_t port=handle->port;
    if(address!=NULL) IPAddress(address,&addr,&port);
    if((hdl->valid==0)||(handle->addr!=addr)||(handle->port!=port))
    {
        handle->wait_time=DFLT;
        mPropertyVariate(obj,"UDP_wait",&(handle->wait_time));
        handle->addr = addr;handle->port=port;
        mException(handle->port==0,EXIT,"invalid UDP port");
        // printf("handle->port = %d\n",handle->port);
        if(hdl->valid==0) {if(UDPSetup(handle)==MORN_FAIL) return NULL;}

        handle->recive_addr.sin_family = AF_INET;
        handle->recive_addr.sin_port = htons(handle->port);
        ADDR(handle->recive_addr.sin_addr) = handle->addr;
        if(bind(handle->udp,(struct sockaddr *)&(handle->recive_addr),sizeof(struct sockaddr))<0) return NULL;

        hdl->valid=1;
    }
    ADDR(handle->recive_addr.sin_addr) = handle->addr;

    if(handle->wait_time>=0)
    {
        struct timeval wait_time;wait_time.tv_sec=handle->wait_time/1000;wait_time.tv_usec=(handle->wait_time%1000)*1000;

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(handle->udp,&fds);
        #if defined(_WIN64)||defined(_WIN32)
        int maxfd=0;
        #else
        int maxfd=handle->udp+1;
        #endif
        int flag = select(maxfd,&fds,NULL,NULL,&wait_time);
        if(flag==0) return NULL;
        mException(flag==SOCKET_ERROR,EXIT,"udp error");
    }
    
    unsigned int len = sizeof(struct sockaddr_in);
    int ret = recvfrom(handle->udp,data,*size,0,(struct sockaddr *)&(handle->recive_addr),(void *)(&len));
    if(ret<*size) ((char *)data)[ret]=0;
    *size = ret;

    uint8_t *s=(uint8_t *)(&(ADDR(handle->recive_addr.sin_addr)));
    sprintf(handle->addr_str,"%d.%d.%d.%d:%d",s[0],s[1],s[2],s[3],handle->port);
    
    return handle->addr_str;
}

struct HandleTCPClient
{
    SOCKET tcp;
    uint32_t addr;
    uint32_t port;
    struct sockaddr_in server_addr;
    
    char addr_str[32];
};
void endTCPClient(struct HandleTCPClient *handle)
{
    closesocket(handle->tcp);
    #if defined _WIN64
    WSACleanup();
    #endif
}

#define HASH_TCPClient 0xc33ebe53
struct HandleTCPClient *m_TCPClient(MObject *obj,const char *server_address)
{
    MHandle *hdl=mHandle(obj,TCPClient);
    struct HandleTCPClient *handle = hdl->handle;
    uint32_t addr=handle->addr,port=handle->port;
    if(server_address!=NULL) IPAddress(server_address,&addr,&port);
    if((hdl->valid==0)||(handle->addr!=addr)||(handle->port!=port))
    {
        handle->addr = addr;handle->port=port;
        if(hdl->valid==0) 
        {
            #if defined _WIN64
            WSADATA wsa;
            WORD version = MAKEWORD(2,2);
            mException(WSAStartup(version,&wsa),EXIT,"tcp error");
            #endif

            handle->tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            mException(handle->tcp<0,EXIT,"tcp error");
        }

        handle->server_addr.sin_family = AF_INET;
        handle->server_addr.sin_port = htons(handle->port);
        ADDR(handle->server_addr.sin_addr) = handle->addr;
        int ret = connect(handle->tcp,(struct sockaddr*)&(handle->server_addr),sizeof(struct sockaddr));
        if(ret<0) return NULL;

        uint8_t *s=(uint8_t *)(&(ADDR(handle->server_addr.sin_addr)));
        sprintf(handle->addr_str,"%d.%d.%d.%d:%d",s[0],s[1],s[2],s[3],handle->port);
        
        hdl->valid=1;
    }
    return handle;
}

char *m_TCPClientWrite(MObject *obj,const char *server_address,void *data,int size)
{
    struct HandleTCPClient *handle = m_TCPClient(obj,server_address);
    if(handle==NULL) return NULL;
    if(size<0) size=strlen(data);
    send(handle->tcp,data,size,0);
    return handle->addr_str;
}
char *m_TCPClientRead(MObject *obj,const char *server_address,void *data,int *size)
{
    struct HandleTCPClient *handle = m_TCPClient(obj,server_address);
    if(handle==NULL) return NULL;
    int ret = recv(handle->tcp,data,*size,0);
    if(ret<0) return NULL;
    if(ret<*size) ((char *)data)[ret]=0;
    *size = ret;return handle->addr_str;
}

struct HandleTCPServer
{
    SOCKET server;
    struct sockaddr_in server_addr;
    uint32_t server_port;
    
    struct sockaddr_in client_addr[128];
    SOCKET client_socket[128];
    int client_order0;
    int client_order1;
    
    char addr_str[32];
};
void endTCPServer(struct HandleTCPServer *handle)
{
    closesocket(handle->server);
    #if defined _WIN64
    WSACleanup();
    #endif
}
#define HASH_TCPServer 0x6fafa477
struct HandleTCPServer *m_TCPServer(MObject *obj,uint32_t port)
{
    MHandle *hdl=mHandle(obj,TCPServer);
    struct HandleTCPServer *handle = hdl->handle;
    if(hdl->valid==0)
    {
        mException(port==0,EXIT,"invalid TCP port");
        handle->server_port=port;
        if(hdl->valid==0) 
        {
            #if defined _WIN64
            WSADATA wsa;
            WORD version = MAKEWORD(2,2);
            mException(WSAStartup(version,&wsa),EXIT,"tcp error");
            #endif
            handle->server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            mException(handle->server<0,EXIT,"tcp error");
            hdl->valid=1;
        }
        handle->server_addr.sin_family = AF_INET;
        handle->server_addr.sin_port = htons(handle->server_port);
        ADDR(handle->server_addr.sin_addr) = htonl(INADDR_ANY);
        mException(bind(handle->server,(struct sockaddr *)&(handle->server_addr),sizeof(struct sockaddr))<0,EXIT,"tcp error");

        listen(handle->server,10);
    }
    
    return handle;
}
char *m_TCPServerRead(MObject *obj,const char *address,void *data,int *size)
{
    uint32_t addr=0,port=0;
    if(address!=NULL) IPAddress(address,&addr,&port);
    struct HandleTCPServer *handle = m_TCPServer(obj,port);
    
    int ret,n;
    for(int i=handle->client_order0;i<handle->client_order0+128;i++)
    {
        n=i%128;
        if(addr>0) {if(memcmp(handle->client_addr+n,&addr,4)!=0) continue;}
        if(handle->client_socket[n]==0) continue;
        ret = recv(handle->client_socket[n],data,*size,0);
        if(ret<=0) continue;
        
        if(ret<*size) ((char *)data)[ret]=0;
        *size = ret;
        handle->client_order0 = n;
        uint8_t *s=(uint8_t *)(&(ADDR(handle->server_addr.sin_addr)));
        sprintf(handle->addr_str,"%d.%d.%d.%d:%d",s[0],s[1],s[2],s[3],handle->server_port);
        return handle->addr_str;
    }

    struct timeval wait_time;memset(&wait_time,0,sizeof(struct timeval));
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(handle->server,&fds);
    #if defined(_WIN64)||defined(_WIN32)
    int maxfd=0;
    #else
    int maxfd=handle->server+1;
    #endif
    int flag = select(maxfd,&fds,NULL,NULL,&wait_time);
    if(flag==0) return NULL;
    mException(flag==SOCKET_ERROR,EXIT,"tcp error");
    
    n=handle->client_order1;if(n==128) n=0;
    handle->client_order1=n+1;
    int addr_size = sizeof(struct sockaddr_in);
    handle->client_socket[n] = accept(handle->server,(struct sockaddr *)(handle->client_addr+n),&addr_size);
    if(handle->client_socket[n]<=0) return NULL;

    FD_ZERO(&fds);
    FD_SET(handle->client_socket[n],&fds);
    #if defined(_WIN64)||defined(_WIN32)
    maxfd=0;
    #else
    maxfd=handle->client_socket[n]+1;
    #endif
    flag = select(maxfd,&fds,NULL,NULL,&wait_time);
    if(flag==0) return NULL;
    mException(flag==SOCKET_ERROR,EXIT,"tcp error");
    
    if(addr>0) {if(memcmp(handle->client_addr+n,&addr,4)!=0) return NULL;}
    ret = recv(handle->client_socket[n],data,*size,0);
    if(ret<0) return NULL;
    
    if(ret<*size) ((char *)data)[ret]=0;
    *size = ret;
    
    uint8_t *s=(uint8_t *)(&(ADDR(handle->server_addr.sin_addr)));
    sprintf(handle->addr_str,"%d.%d.%d.%d:%d",s[0],s[1],s[2],s[3],handle->server_port);
    return handle->addr_str;
}

char *m_TCPServerWrite(MObject *obj,const char *address,void *data,int size)
{
    uint32_t addr=0,port=0;
    if(address!=NULL) IPAddress(address,&addr,&port);
    mException(addr==0,EXIT,"invalid input");
    struct HandleTCPServer *handle = m_TCPServer(obj,port);

    int n;
    for(int i=handle->client_order0;i<handle->client_order0+128;i++)
    {
        n=i%128;
        if(memcmp(handle->client_addr+n,&addr,4)!=0) continue;
        if(handle->client_socket[n]==0) continue;
        
        send(handle->client_socket[n],data,size,0);

        handle->client_order0 = n;
        uint8_t *s=(uint8_t *)(&(ADDR(handle->server_addr.sin_addr)));
        sprintf(handle->addr_str,"%d.%d.%d.%d:%d",s[0],s[1],s[2],s[3],handle->server_port);
        return handle->addr_str;
    }

    struct timeval wait_time;memset(&wait_time,0,sizeof(struct timeval));
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(handle->server,&fds);
    #if defined(_WIN64)||defined(_WIN32)
    int maxfd=0;
    #else
    int maxfd=handle->server+1;
    #endif
    int flag = select(maxfd,&fds,NULL,NULL,&wait_time);
    if(flag==0) return NULL;
    mException(flag==SOCKET_ERROR,EXIT,"tcp error");

    n=handle->client_order1;if(n==128) n=0;
    handle->client_order1=n+1;
    int addr_size = sizeof(struct sockaddr_in);
    handle->client_socket[n] = accept(handle->server,(struct sockaddr *)(handle->client_addr+n),&addr_size);
    if(handle->client_socket[n]<=0) return NULL;

    FD_ZERO(&fds);
    FD_SET(handle->client_socket[n],&fds);
    #if defined(_WIN64)||defined(_WIN32)
    maxfd=0;
    #else
    maxfd=handle->client_socket[n]+1;
    #endif
    flag = select(maxfd,&fds,NULL,NULL,&wait_time);
    if(flag==0) return NULL;
    mException(flag==SOCKET_ERROR,EXIT,"tcp error");

    if(memcmp(handle->client_addr+n,&addr,4)!=0) return NULL;
    send(handle->client_socket[n],data,size,0);
    
    uint8_t *s=(uint8_t *)(&(ADDR(handle->server_addr.sin_addr)));
    sprintf(handle->addr_str,"%d.%d.%d.%d:%d",s[0],s[1],s[2],s[3],handle->server_port);
    return handle->addr_str;
}


#endif
