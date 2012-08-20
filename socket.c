#include <lua.h>
#include <lauxlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SOCKET_NAME     "SOCKET*"

struct socket {
    int fd;
    int timeout;
};

#define tolsocket(L) ((struct socket *)luaL_checkudata(L, 1, SOCKET_NAME));

static const char *
__socket_gaistrerror(int err)
{
    if (err == 0)
        return NULL;
    switch (err) {
    case EAI_AGAIN:
        return "temporary failure in name resolution";
    case EAI_BADFLAGS:
        return "invalid value for ai_flags";
#ifdef EAI_BADHINTS
    case EAI_BADHINTS:
        return "invalid value for hints";
#endif
    case EAI_FAIL:
        return "non-recoverable failure in name resolution";
    case EAI_FAMILY:
        return "ai_family not supported";
    case EAI_MEMORY:
        return "memory allocation failure";
    case EAI_NONAME:
        return "host or service not provided, or not known";
    case EAI_OVERFLOW:
        return "argument buffer overflow";
#ifdef EAI_PROTOCOL
    case EAI_PROTOCOL:
        return "resolved protocol is unknown";
#endif
    case EAI_SERVICE:
        return "service not supported for socket type";
    case EAI_SOCKTYPE:
        return "ai_socktype not supported";
    case EAI_SYSTEM:
        return strerror(errno);
    default:
        return gai_strerror(err);
    }
}

/**
 * sock = socket.socket(family, type)
 */
static int
socket_socket(lua_State * L)
{
    int family = (int)luaL_checknumber(L, 1);
    int type = (int)luaL_checknumber(L, 2);
    struct socket *s =
        (struct socket *)lua_newuserdata(L, sizeof(struct socket));
    s->fd = socket(family, type, 0);
    luaL_setmetatable(L, SOCKET_NAME);
    return (s->fd > 0) ? 1 : 0;
}

/**
 * addrinfos, err = socket.getaddrinfo(host, port[, family=0[, socktype=0[, protocol=0[, flags=0]]]])
 *
 * Translate the host/port argument into a sequence of table that contain all
 * the necessary arguments for creating a socket to connect to that service,
 * each table has following 5 fileds:
 *  family
 *  socktype
 *  protocol
 *  canonname
 *  addr
 *
 * `host` is a domain name, a string representation of an IPv4/v6 address or nil
 * `port` is a string service name such as 'http', a numeric port number or nil
 * The `family`, `socktype`, `protocol` arguments can be optionally specified in
 * order to narrow the list of addresses returned. Passing zero (default) for
 * each of these arguments selects full range of results.
 * The `flags` can be one or serveral of the AI_* constants, and will influence
 * how results are computed and returned. For example, AI_NUMERICHOST will
 * disable domain name resolution and will raise an error if host is a domain
 * name.
 *  
 */
static int
socket_getaddrinfo(lua_State * L)
{
    const char *hostname = luaL_checkstring(L, 1);
    int type = lua_type(L, 2);
    char pbuf[30];
    char *pptr;
    switch (type) {
    case LUA_TNUMBER:
        snprintf(pbuf, sizeof(pbuf), "%d", (int)luaL_checknumber(L, 2));
        pptr = pbuf;
        break;
    case LUA_TSTRING:
        pptr = (char *)luaL_checkstring(L, 2);
        break;
    case LUA_TNIL:
        pptr = (char *)NULL;
        break;
    default:
        lua_pushnil(L);
        lua_pushstring(L, "port argument expect string/number/nil");
        return 2;
        break;
    }
    int family = luaL_optnumber(L, 3, AF_UNSPEC);
    int socktype = luaL_optnumber(L, 4, 0);
    int protocol = luaL_optnumber(L, 5, 0);
    int flags = luaL_optnumber(L, 6, 0);

    struct addrinfo *iterator = NULL, *resolved = NULL;
    struct addrinfo hints;
    int ret = 0;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = socktype;
    hints.ai_protocol = protocol;
    hints.ai_flags = flags;
    ret = getaddrinfo(hostname, pptr, &hints, &resolved);
    if (ret != 0) {
        lua_pushnil(L);
        lua_pushstring(L, __socket_gaistrerror(ret));
        return 2;
    }
    lua_newtable(L);
    int i = 1;
    for (iterator = resolved; iterator; iterator = iterator->ai_next) {
        char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
        getnameinfo(iterator->ai_addr, iterator->ai_addrlen, hbuf, sizeof(hbuf),
                    sbuf, 0, flags);
        lua_pushnumber(L, i);
        lua_newtable(L);

        lua_pushliteral(L, "family");
        lua_pushnumber(L, iterator->ai_family);
        lua_settable(L, -3);
        lua_pushliteral(L, "socktype");
        lua_pushnumber(L, iterator->ai_socktype);
        lua_settable(L, -3);
        lua_pushliteral(L, "protocol");
        lua_pushnumber(L, iterator->ai_protocol);
        lua_settable(L, -3);
        lua_pushliteral(L, "canonname");
        lua_pushstring(L, iterator->ai_canonname ? iterator->ai_canonname : "");
        lua_settable(L, -3);
        lua_pushliteral(L, "addr");
        lua_pushstring(L, hbuf);
        lua_settable(L, -3);

        lua_settable(L, -3);
        i++;
    }
    freeaddrinfo(resolved);
    return 1;
}

/*const char **/
/*inet_try_connect(int fd, const char *address, int port, int timeout, struct addrinfo *connecthints)*/
/*{*/
/*struct addrinfo *iterator = NULL, *resolved = NULL;*/
/*const char *err = NULL;*/
/*err = __socket_gaistrerror(getaddrinfo(address, */
/*}*/

/**
 * ok, err = sock:connect(host, port)
 * 
 * Attemps to connect to TCP socket object to a remote server.
 */
static int
sock_connect(lua_State * L)
{
    struct socket *s = tolsocket(L);
    const char *host = luaL_checkstring(L, 2);
    int port = (int)luaL_checknumber(L, 3);

    struct hostent *hostinfo;
    hostinfo = gethostbyname(host);
    if (hostinfo == NULL) {
        return luaL_error(L, "Invalid host to connect: %s", host);
    }
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(struct sockaddr_in));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr = *(struct in_addr *)hostinfo->h_addr;

    int r =
        connect(s->fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
    if (r < 0) {
        return luaL_error(L, "Cannot connect to %s:%d, error: %s", host, port,
                          strerror(errno));
    }

    return 0;
}

/**
 * bytes, err = sock:send(data)
 *
 * Sends data.
 */
static int
sock_send(lua_State * L)
{
    struct socket *s = tolsocket(L);
    size_t size;
    const char *buf = luaL_checklstring(L, 2, &size);
    ssize_t n = send(s->fd, buf, size, 0);
    if (n < 0) {
        return luaL_error(L, "Cannot send data, error: %s", strerror(errno));
    }

    lua_pushinteger(L, n);
    return 1;
}

/**
 * sock:recv(bufsize[, flags])
 *
 * Receive data from the socket. The return value is a string representing the data received. The maximum amount of data to be received at once is specified by bufsize.
 */
static int
sock_recv(lua_State * L)
{
    struct socket *s = tolsocket(L);
    size_t bufsize = (int)luaL_checknumber(L, 2);
    int flags = luaL_optnumber(L, 3, 0);
    char *buf = malloc(bufsize * sizeof(char));
    for (;;) {
        int bytes = recv(s->fd, buf, bufsize, flags);
        if (bytes > 0) {
            lua_pushlstring(L, buf, bytes);
            return 1;
        } else if (bytes == 0) {
            lua_pushnil(L);
            return 1;
        } else {
            switch (errno) {
            case EINTR:
            case EAGAIN:
                continue;
            default:
                return luaL_error(L, "Cannot recv data, error: %s",
                                  strerror(errno));
            }
        }
    }
}

/**
 * sock:close
 *
 * Set the file descriptor to -1 so operations tried subsequently will surely
 * fail.
 */
static int
sock_close(lua_State * L)
{
    struct socket *s = tolsocket(L);

    int fd;

    if ((fd = s->fd) != -1) {
        s->fd = -1;
        close(fd);
    }

    return 0;
}

/**
 * sock:settimeout(timeout)
 *
 * Set the timeout value in milliseconds for subsequent socket operations
 * (read/recv, etc).
 */
static int
sock_settimeout(lua_State * L)
{
    struct socket *s = tolsocket(L);
    int timeout = (int)luaL_checknumber(L, 2);
    if (timeout > 0) {
        s->timeout = timeout;
    }
    return 0;
}

/**
 * sock:setblocking(block)
 *
 * 1/true   - blocking mode
 * 0/false  - non-blocking mode
 */
static int
sock_setblocking(lua_State * L)
{
    struct socket *s = tolsocket(L);
    int block = (int)luaL_checknumber(L, 2);
    s->timeout = block ? -1 : 0;

    int flags = fcntl(s->fd, F_GETFL, 0);
    if (block) {
        flags &= (~O_NONBLOCK);
    } else {
        flags |= O_NONBLOCK;
    }
    fcntl(s->fd, F_SETFL, flags);
    return 0;
}

static const luaL_Reg socketlib[] = {
    {"socket", socket_socket},
    {"getaddrinfo", socket_getaddrinfo},
    {NULL, NULL},
};

static const luaL_Reg sock_methods[] = {
    {"connect", sock_connect},
    {"send", sock_send},
    {"recv", sock_recv},
    {"close", sock_close},
    {"settimeout", sock_settimeout},
    {"setblocking", sock_setblocking},
    {NULL, NULL},
};

int
luaopen_socket_c(lua_State * L)
{
    luaL_checkversion(L);
    luaL_newlib(L, socketlib);

#define ADD_NUM_CONST(name)     \
    lua_pushnumber(L, name);  \
    lua_setfield(L, -2, # name)

    // These constants represent the address (and protocol) families, used for the first argument to socket().
    ADD_NUM_CONST(AF_INET);
    ADD_NUM_CONST(AF_INET6);
    ADD_NUM_CONST(AF_UNIX);

    // These constants represent the socket types, used for the second argument to socket(). (Only SOCK_STREAM and SOCK_DGRAM appear to be generally useful.)
    ADD_NUM_CONST(SOCK_STREAM);
    ADD_NUM_CONST(SOCK_RAW);
    ADD_NUM_CONST(SOCK_RDM);
    ADD_NUM_CONST(SOCK_SEQPACKET);

    // AI_*
    ADD_NUM_CONST(AI_PASSIVE);
    ADD_NUM_CONST(AI_CANONNAME);
    ADD_NUM_CONST(AI_NUMERICHOST);
    ADD_NUM_CONST(AI_NUMERICSERV);
    ADD_NUM_CONST(AI_MASK);
    ADD_NUM_CONST(AI_ALL);
    ADD_NUM_CONST(AI_V4MAPPED_CFG);
    ADD_NUM_CONST(AI_ADDRCONFIG);
    ADD_NUM_CONST(AI_V4MAPPED);
    ADD_NUM_CONST(AI_DEFAULT);

    // Create a metatable for socket userdata.
    luaL_newmetatable(L, SOCKET_NAME);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");     /* metable.__index = metatable */
    luaL_setfuncs(L, sock_methods, 0);
    lua_pop(L, 1);

    return 1;
}
