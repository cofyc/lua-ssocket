#include "compat.h"

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
#include <sys/time.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <poll.h>
#include <signal.h>
#include "timeout.h"
#include "buffer.h"

#define TCPSOCK_TYPENAME     "TCPSOCKET*"
#define UDPSOCK_TYPENAME     "UDPSOCKET*"

/* Socket address */
typedef union {
  struct sockaddr sa;
  struct sockaddr_in in;
  struct sockaddr_un un;
} sockaddr_t;

/* Convert "sockaddr_t" to "struct sockaddr *". */
#define SAS2SA(x) (&((x)->sa))

/* Socket Object */
struct sockobj {
    int fd;
    int sock_family;
    double sock_timeout;        /* in seconds */
    struct buffer *buf;         /* used for buffer reading */
};

#define getsockobj(L) ((struct sockobj *)lua_touserdata(L, 1));
#define CHECK_ERRNO(expected)   (errno == expected)

/* Custom socket error strings */
#define ERROR_TIMEOUT   "Operation timed out"
#define ERROR_CLOSED    "Connection closed"
#define ERROR_REFUSED   "Connection refused"

/* Options */
#define OPT_TCP_NODELAY   "tcp_nodelay"
#define OPT_TCP_KEEPALIVE "tcp_keepalive"

#define SEND_BUFSIZE 8192
#define RECV_BUFSIZE 8192

/**
 * Function to perform the setting of socket blocking mode.
 */
static void
__setblocking(int fd, int block)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (block) {
        flags &= (~O_NONBLOCK);
    } else {
        flags |= O_NONBLOCK;
    }
    fcntl(fd, F_SETFL, flags);
}

/**
 * Do a event polling on the socket, if necessary (sock_timeout > 0).
 *
 * Returns:
 *  1   on timeout
 *  -1  on error
 *  0   success
 */
#define EVENT_NONE      0
#define EVENT_READABLE  POLLIN
#define EVENT_WRITABLE  POLLOUT
#define EVENT_ANY       (POLLIN | POLLOUT)
static int
__waitfd(struct sockobj *s, int event, struct timeout *tm)
{
    int ret;

    // Nothing to do if we're not in timeout mode.
    if (s->sock_timeout <= 0.0)
        return 0;

    // Nothing to do if socket is closed.
    if (s->fd < 0)
        return 0;

    struct pollfd pollfd;
    pollfd.fd = s->fd;
    pollfd.events = event;

    do {
        // Handling this condition here simplifies the loops.
        double timeout = timeout_left(tm);
        if (timeout <= 0)
            return 1;
        ret = poll(&pollfd, 1, (int)(timeout * 1e3));
    } while (ret == -1 && CHECK_ERRNO(EINTR));

    if (ret < 0) {
        return -1;
    } else if (ret == 0) {
        return 1;
    } else {
        return 0;
    }
}

int
__select(int nfds, fd_set * readfds, fd_set * writefds, fd_set * errorfds,
         struct timeout *tm)
{
    int ret;
    do {
        struct timeval tv = { 0, 0 };
        double t = timeout_left(tm);
        if (t >= 0) {
            tv.tv_sec = (int)t;
            tv.tv_usec = (int)((t - tv.tv_sec) * 1.0e6);
        }

        ret = select(nfds, readfds, writefds, errorfds, (t >= 0) ? &tv : NULL);
    } while (ret < 0 && errno == EINTR);
    return ret;
}

/* 
 * Convert a string specifying a host name or one of a few symbolic names to a
 * numeric IP address.
 */
static int
__setipaddr(lua_State *L, const char *name, struct sockaddr *addr_ret, size_t addr_ret_size, int af)
{
    struct addrinfo hints, *res;
    int err;
    int d1, d2, d3, d4;
    char ch;
    memset((void *)addr_ret, 0, addr_ret_size);
    
    if (sscanf(name, "%d.%d.%d.%d%c", &d1, &d2, &d3, &d4, &ch) == 4
        && 0 <= d1 && d1 <= 255
        && 0 <= d2 && d2 <= 255
        && 0 <= d3 && d3 <= 255
        && 0 <= d4 && d4 <= 255) {
        struct sockaddr_in *sin;
        sin = (struct sockaddr_in *)addr_ret;
        sin->sin_addr.s_addr = htonl(((long)d1 << 24) | ((long)d2 << 16) | ((long)d3 << 8) | ((long)d4 << 0));
        sin->sin_family = AF_INET;
        sin->sin_len = sizeof(*sin);
        return 0;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = af;
    err = getaddrinfo(name, NULL, &hints, &res);
    if (err) {
        lua_pushnil(L);
        lua_pushstring(L, gai_strerror(errno));
        return -1;
    }
    if (res->ai_addrlen < addr_ret_size)
        addr_ret_size = res->ai_addrlen;
    memcpy((char *)addr_ret, res->ai_addr, addr_ret_size);
    freeaddrinfo(res);
    return 0;
}

/**
 * Parse socket address arguments.
 *
 * Socket addresses are represented as follows:

 *  - A single string is used for the AF_UNIX address family.
 *  - Two arguments (host, port) is used for the AF_INET address family,
 *    where host is a string representing either a hostname in Internet Domain
 *    Notation like 'www.example.com' or an IPv4 address like '8.8.8.8', and port
 *    is an number.

 * If you use a hostname in the host portion of IPv4/IPv6 socket address, the
 * program may show a nodeterministic behavior, as we use the first address
 * returned from the DNS resolution. The socket address will be resolved
 * differenlty into an actual IPv4/v6 address, depending on the results from DNS
 * resolution and/or the host configuration. For deterministic behavior use a
 * numeric address in host portion.
 *
 * This method assumed that address arguments start at argument index 2.
 *
 * Returns 0 on success, 1 on failure.
 */
static int
__getsockaddrfromarg(lua_State * L, struct sockobj *s, struct sockaddr *addr_ret,
                     socklen_t * len_ret)
{
    if (s->sock_family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in *)addr_ret;
        const char *host;
        int port;
        host = luaL_checkstring(L, 2);
        port = luaL_checknumber(L, 3);
        if (__setipaddr(L, host, (struct sockaddr *)addr, sizeof(*addr), AF_INET) != 0) {
            return 2;
        }
        addr->sin_family = AF_INET;
        addr->sin_port = htons(port);
        *len_ret = sizeof(*addr);
    } else if (s->sock_family == AF_UNIX) {
        struct sockaddr_un *addr = (struct sockaddr_un *)addr_ret;
        const char *path = luaL_checkstring(L, 2);
        addr->sun_family = AF_UNIX;
        strncpy(addr->sun_path, path, sizeof(addr->sun_path) - 1);
        *len_ret = sizeof(*addr);
    } else {
        assert(0);
    }
    return 0;
}

/**
 * Create a table, push address info into it.
 *
 * The family field os the socket object is inspected to determine what kind of
 * address it really is.
 *
 * In case of success, a table associated with address info pushed on the stack;
 * In case of error, a nil value with a string describing the error pushed on
 * the stack.
 */
static int
__makesockaddr(lua_State * L, struct sockobj *s, struct sockaddr *addr,
               socklen_t addrlen)
{
    lua_newtable(L);
    if (addrlen == 0) {
        return 1;
    }

    switch (addr->sa_family) {
    case AF_INET:
        {
            struct sockaddr_in *a = (struct sockaddr_in *)addr;
            char buf[NI_MAXHOST];
            int err = getnameinfo(addr, addrlen, buf, sizeof(buf), NULL, 0,
                                  NI_NUMERICHOST);
            if (err) {
                err = errno;
                lua_pushnil(L);
                lua_pushstring(L, gai_strerror(errno));
                return 2;
            }
            lua_pushnumber(L, 1);
            lua_pushstring(L, buf);
            lua_settable(L, -3);
            lua_pushnumber(L, 2);
            lua_pushnumber(L, ntohs(a->sin_port));
            lua_settable(L, -3);
            return 1;
        }
    case AF_UNIX:
        {
            struct sockaddr_un *a = (struct sockaddr_un *)addr;
#ifdef linux
            if (a->sun_path[0] == 0) { /* Linux abstract namespace */
                addrlen -= offset(struct sockaddr_un, sun_path);
                lua_pushlstring(L, a->sun_path, addrlen);
            } else
#endif
            {
                /* regular NULL-terminated string */
                lua_pushstring(L, a->sun_path);
            }
            return 1;
        }
    default:
        /* If we don't know the address family, return it as an {int, bytes}
         * table. */
        lua_pushnumber(L, 1);
        lua_pushnumber(L, addr->sa_family);
        lua_settable(L, -3);
        lua_pushnumber(L, 2);
        lua_pushstring(L, addr->sa_data);
        lua_settable(L, -3);
        return 1;
    }
}

/**
 * Get the address length according to the socket object's address family.
 * Return 1 if the family is known, 0 otherwise. The length is returned through
 * len_ret.
 */
static int
__getsockaddrlen(struct sockobj *s, socklen_t * len_ret)
{
    switch (s->sock_family) {
    case AF_UNIX:
        *len_ret = sizeof(struct sockaddr_un);
        return 1;
    case AF_INET:
        *len_ret = sizeof(struct sockaddr_in);
        return 1;
    case AF_INET6:
        *len_ret = sizeof(struct sockaddr_in6);
        return 1;
    default:
        return 0;
    }
}

/**
 * Create socket object.
 */
struct sockobj *
__sockobj_create(lua_State *L, const char *tname)
{
    struct sockobj *s =
        (struct sockobj *)lua_newuserdata(L, sizeof(struct sockobj));
    if (!s) {
        return NULL;
    }
    s->fd = -1;
    s->sock_timeout = -1;
    s->sock_family = 0;
    s->buf = NULL;
    luaL_setmetatable(L, tname);
    return s;
}

/**
 * Generic socket creation.
 */
static int
__sockobj_createsocket(lua_State *L, struct sockobj *s, int type)
{
    int fd, on = 1;

    if ((fd = socket(s->sock_family, type, 0)) == -1) {
        lua_pushnil(L);
        lua_pushfstring(L, "creating socket: %s", strerror(errno));
        return -1;
    }

    s->fd = fd;

    // reuse address
    if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        lua_pushnil(L);
        lua_pushfstring(L, "setsockopt SO_REUSEADDR: %s", strerror(errno));
        return -1;
    }

    // block or non-block
    __setblocking(s->fd, s->sock_timeout < 0.0);

    return 0;
}

/**
 * Close associated socket and buffers.
 */
static int
__sockobj_close(lua_State *L, struct sockobj *s)
{
    if (s->fd != -1) {
        if (close(s->fd) != 0) {
            lua_pushnil(L);
            lua_pushstring(L, strerror(errno));
            return -1;
        }
        s->fd = -1;
    }
    if (s->buf) {
        buffer_delete(s->buf);
        s->buf = NULL;
    }
    return 0;
}

/**
 * Generic socket connection.
 */
static int
__sockobj_connect(lua_State *L, struct sockobj *s, struct sockaddr *addr, socklen_t len)
{
    int ret;
    char *errstr = NULL;
    struct timeout tm;
    timeout_init(&tm, s->sock_timeout);

    errno = 0;
    ret = connect(s->fd, addr, len);

    if (s->sock_timeout > 0.0 && CHECK_ERRNO(EINPROGRESS)) {
        /* Connecting in progress with timeout, wait until we have the result of
         * the connection attempt or timeout.
         */
        int timeout = __waitfd(s, EVENT_ANY, &tm);
        if (timeout == 1) {
            errstr = ERROR_TIMEOUT;
            goto err;
        } else if (timeout == 0) {
            // In case of EINPROGRESS, use getsockopt(SO_ERROR) to get the real
            // error, when the connection attempt finished.
            socklen_t ret_size = sizeof(ret);
            getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &ret, &ret_size);
            if (ret == EISCONN) {
                errno = 0;
            } else {
                errno = ret;
            }
        } else {
            errstr = strerror(errno);
            goto err;
        }
    }

    if (errno) {
        errstr = strerror(errno);
        goto err;
    }
    return 0;

err:
    assert(errstr);
    __sockobj_close(L, s);
    lua_pushnil(L);
    lua_pushstring(L, errstr);
    return -1;
}

/**
 * tcpsock, err = socket.tcp()
 *
 * Creates and returns a TCP or stream-oriented unix domain socket object.
 */
static int
socket_tcp(lua_State * L)
{
    struct sockobj *s = __sockobj_create(L, TCPSOCK_TYPENAME);
    if (!s) {
        return luaL_error(L, "out of memory");
    }
    return 1;
}

/**
 * udpsock, err = socket.udp()
 *
 * Creates and returns a UDP or datagram-oriented unix domain socket object.
 */
static int
socket_udp(lua_State * L)
{
    struct sockobj *s = __sockobj_create(L, UDPSOCK_TYPENAME);
    if (!s) {
        return luaL_error(L, "out of memory");
    }
    return 1;
}

static void
__collect_fds(lua_State * L, int tab, fd_set * set, int *max_fd)
{
    if (lua_isnil(L, tab))
        return;

    luaL_checktype(L, tab, LUA_TTABLE);

    int i = 1;
    while (1) {
        int fd = -1;
        lua_pushnumber(L, i);
        lua_gettable(L, tab);   // get ith fd

        if (lua_isnil(L, -1)) {
            // end of table loop
            lua_pop(L, 1);
            break;
        }

        if (lua_isnumber(L, -1)) {
            fd = lua_tonumber(L, -1);
            if (fd < 0) {
                fd = -1;
            }
        } else {
            // ignore
            lua_pop(L, 1);
            continue;
        }

        if (fd != -1) {
            if (fd >= FD_SETSIZE) {
                luaL_argerror(L, tab, "descriptor too large for set size");
            }
            if (*max_fd < fd) {
                *max_fd = fd;
            }
            FD_SET(fd, set);
        }

        lua_pop(L, 1);
        i++;
    }
}

static void
__return_fd(lua_State * L, fd_set * set, int max_fd)
{
    int fd;
    int i = 1;
    lua_newtable(L);
    for (fd = 0; fd < max_fd; fd++) {
        if (FD_ISSET(fd, set)) {
            lua_pushnumber(L, i);
            lua_pushnumber(L, fd);
            lua_settable(L, -3);
        }
    }
}

/**
 * readfds, writefds, err = socket.select(readfds, writefds[, timeout=-1])
 *
 *  `readfds`, `writefds` are all table of fds (which was returned from
 *  sockobj:fileno()).
 */
static int
socket_select(lua_State * L)
{
    int max_fd = -1;
    fd_set rset, wset;
    struct timeout tm;
    double timeout = luaL_optnumber(L, 3, -1);
    timeout_init(&tm, timeout);
    FD_ZERO(&rset);
    FD_ZERO(&wset);
    __collect_fds(L, 1, &rset, &max_fd);
    __collect_fds(L, 2, &wset, &max_fd);

    int ret = __select(max_fd + 1, &rset, &wset, NULL, &tm);
    if (ret > 0) {
        __return_fd(L, &rset, max_fd + 1);
        __return_fd(L, &wset, max_fd + 1);
        return 2;
    } else if (ret == 0) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushstring(L, ERROR_TIMEOUT);
        return 3;
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 3;
    }
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
        lua_pushstring(L, gai_strerror(ret));
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

/*** sock_* methods are common to tcpsocket or udpsocket ***/

/**
 * fd = sockobj:fileno()
 *
 * Return the integer file descriptor of the socket.
 */
static int
sockobj_fileno(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    lua_pushnumber(L, s->fd);
    return 1;
}

/**
 * ok, err = sockobj:close()
 *
 * Close the socket.
 */
static int
sockobj_close(lua_State * L)
{
    struct sockobj *s = getsockobj(L);

    if (__sockobj_close(L, s) == -1)
        return 2;

    lua_pushboolean(L, 1);
    return 1;
}

static int
sockobj_tostring(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    assert(lua_getmetatable(L, -1));
    luaL_getmetatable(L, TCPSOCK_TYPENAME);
    if (lua_rawequal(L, -1, -2)) {
        lua_pop(L, 2);
        lua_pushfstring(L, "<tcpsock: %d>", s->fd);
    } else {
        lua_pop(L, 2);
        lua_pushfstring(L, "<udpsock: %d>", s->fd);
    }
    return 1;
}

/**
 * sockobj:settimeout(timeout)
 *
 * Set the timeout value in seconds for subsequent socket operations.
 *
 * A socket object can be in one of three modes: blocking, non-blocking, or
 * timeout. 
 *  - In blocking mode, operations block untile complete or the system returns an
 *  error.
 *  - In non-blocking mode, operations fail (with an error that is unfortunately
 *  system-dependent) if they cannot be completed immediately.
 *  - In timeout mode, operations fail if they cannot be completed within the
 *  timeout specified for the socket or if the system returns an error. (At the
 *  os level, sockets in timeout mode are internally set in non-blocking mode.
 *
 * Sockets are by default created in blocking mode.
 *
 * Argument:
 *  < 0     -- blocking mode
 *  = 0     -- non-blocking mode
 *  > 0     -- timeout mode (also non-blocking mode with timeout specified)
 */
static int
sockobj_settimeout(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    double timeout = (double)luaL_checknumber(L, 2);
    s->sock_timeout = timeout;
    if (s->fd > 0) {
        __setblocking(s->fd, timeout < 0.0);
    }
    return 0;
}

/*
 * timeout = sockobj:gettimeout()
 *
 * Returns the timeout in seconds (number) associated with socket.
 * A negative timeout indicates that timeout is disabled.
 */
static int
sockobj_gettimeout(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    lua_pushnumber(L, s->sock_timeout);
    return 1;
}

/**
 * ok, err = tcpsock:connect(host, port)
 * ok, err = tcpsock:connect("unix:/path/to/unix-domain.sock")
 *
 * Attemps to connect to TCP socket object to a remote server or to a stream
 * unix domain socket file.
 */
static int
tcpsock_connect(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    sockaddr_t addr;
    socklen_t len;
    int n;
    n = lua_gettop(L);
    if (n != 2 && n != 3) {
        return luaL_error(L, "tcpsocket:connect expecting 2 or 3 arguments"
        " (including the object itself), but seen %d", n);
    }

    if (n == 3) {
        s->sock_family = AF_INET;
    } else if (n == 2) {
        s->sock_family = AF_UNIX;
    }

    if (__getsockaddrfromarg(L, s, SAS2SA(&addr), &len)) {
        return 2;
    }

    if (__sockobj_createsocket(L, s, SOCK_STREAM) == -1)
        return 2;

    if (__sockobj_connect(L, s, SAS2SA(&addr), len) == -1)
        return 2;

    lua_pushboolean(L, 1);
    return 1;
}

/**
 * ok, err = tcpsock:bind(host, port)
 * ok, err = tcpsock:connect("unix:/path/to/unix-domain.sock")
 */
static int
tcpsock_bind(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    sockaddr_t addr;
    socklen_t len;
    int ret;
    char *errstr = NULL;
    int n;
    n = lua_gettop(L);
    if (n != 2 && n != 3) {
        return luaL_error(L, "tcpsocket:bind expecting 2 or 3 arguments"
        " (including the object itself), but seen %d", n);
    }

    if (n == 3) {
        s->sock_family = AF_INET;
    } else if (n == 2) {
        s->sock_family = AF_UNIX;
    }

    if (__getsockaddrfromarg(L, s, SAS2SA(&addr), &len)) {
        return 2;
    }

    if (__sockobj_createsocket(L, s, SOCK_STREAM) == -1)
        return 2;

    ret = bind(s->fd, SAS2SA(&addr), len);
    if (ret < 0) {
        errstr = strerror(errno);
        goto err;
    }

    lua_pushboolean(L, 1);
    return 1;

err:
    assert(errstr);
    lua_pushnil(L);
    lua_pushstring(L, errstr);
    return 2;
}

/**
 * ok, err = tcpsock:listen(backlog)
 *
 * Listen for connections make to the socket.
 * The backlog argument specifies the maximum number of queue connections and
 * should be at least 0.
 */
static int
tcpsock_listen(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    int backlog;
    int ret;
    char *errstr = NULL;
    backlog = luaL_checknumber(L, 2);
    /* To avoid problems on systems that don't allow a negative backlog, force
     * minimu value of 0. */
    if (backlog < 0) {
        backlog = 0;
    }
    ret = listen(s->fd, backlog);
    if (ret < 0) {
        errstr = strerror(ret);
        goto err;
    }

    lua_pushboolean(L, 1);
    return 1;

err:
    assert(errstr);
    lua_pushnil(L);
    lua_pushstring(L, errstr);
    return 2;
}

/**
 * sock, err = tcpsock:accept()
 *
 * Accept a connection. The socket must be bound to an address and listening for
 * connections.
 *
 * In case of success, it returns a socket object usable to read/write data on
 * the connection. Otherwise, it returns nil and a string describing the error.
 */
static int
tcpsock_accept(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    sockaddr_t addr;
    socklen_t addrlen;
    int clientfd;
    char *errstr = NULL;

    if (!__getsockaddrlen(s, &addrlen)) {
        errstr = strerror(errno);
        goto err;
    }

    struct timeout tm;
    timeout_init(&tm, s->sock_timeout);
    int timeout = __waitfd(s, EVENT_READABLE, &tm);
    if (timeout == -1) {
        errstr = strerror(errno);
        goto err;
    } else if (timeout == 1) {
        errstr = ERROR_TIMEOUT;
        goto err;
    } else {
        clientfd = accept(s->fd, SAS2SA(&addr), &addrlen);
        if (clientfd == -1) {
            errstr = strerror(errno);
            goto err;
        }
    }

    struct sockobj *client = __sockobj_create(L, TCPSOCK_TYPENAME);
    client->fd = clientfd;
    client->sock_family = s->sock_family;
    if (!client) {
        return luaL_error(L, "out of memory");
    }
    return 1;

err:
    assert(errstr);
    lua_pushnil(L);
    lua_pushstring(L, errstr);
    return 2;
}

/**
 * bytes, err = tcpsock:write(data)
 *
 * This method is a synchronous operation that will not return until all the
 * data has been flushed into the system socket send buffer or an error occurs.
 *
 * In case of success, it returns the total number of bytes that have been sent.
 * Otherwise, it returns nil and a string describing the error.
 */
static int
tcpsock_write(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    size_t len, total_sent = 0;
    const char *buf = luaL_checklstring(L, 2, &len);
    char *errstr;

    if (s->fd == -1) {
        errstr = ERROR_CLOSED;
        goto err;
    }

    struct timeout tm;
    timeout_init(&tm, s->sock_timeout);
    while (1) {
        errno = 0;
        int timeout = __waitfd(s, EVENT_WRITABLE, &tm);
        if (timeout == -1) {
            errstr = strerror(errno);
            goto err;
        } else if (timeout == 1) {
            errstr = ERROR_TIMEOUT;
            goto err;
        } else {
            size_t send_size = len - total_sent;
            if (send_size > SEND_BUFSIZE) {
                send_size = SEND_BUFSIZE;
            }
            int n = send(s->fd, buf + total_sent, send_size, 0);
            if (n < 0) {
                switch (errno) {
                case EINTR:
                case EAGAIN:
                    continue;
                case EPIPE:
                    // EPIPE means the connection was closed.
                    errstr = ERROR_CLOSED;
                    goto err;
                default:
                    errstr = strerror(errno);
                    goto err;
                }
            } else {
                total_sent += n;
                if (len - total_sent <= 0) {
                    break;
                }
            }
        }
    }

    assert(total_sent == len);
    lua_pushinteger(L, total_sent);
    return 1;

err:
    assert(errstr);
    lua_pushnil(L);
    lua_pushstring(L, errstr);
    return 2;
}

/**
 * data, err, partial = tcpsock:read(size)
 */
static int
tcpsock_read(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    size_t size = (int)luaL_checknumber(L, 2);
    char *errstr = NULL;
    struct buffer *buf = NULL;

    if (s->buf == NULL) {
        s->buf = buffer_create(RECV_BUFSIZE);
    }
    buf = s->buf;

    if (s->fd == -1) {
        errstr = ERROR_CLOSED;
        goto err;
    }

    struct timeout tm;
    timeout_init(&tm, s->sock_timeout);

again:
    if (buffer_size(buf) >= size) {
        goto success;
    }

    while (1) {
        errno = 0;
        int timeout = __waitfd(s, EVENT_READABLE, &tm);
        if (timeout == -1) {
            errstr = strerror(errno);
            goto err;
        } else if (timeout == 1) {
            errstr = ERROR_TIMEOUT;
            goto err;
        } else {
            if (buffer_available(buf) < RECV_BUFSIZE) {
                buffer_grow(buf, RECV_BUFSIZE - buffer_available(buf));
            }
            int bytes_read = recv(s->fd, buf->last, RECV_BUFSIZE, 0);
            if (bytes_read > 0) {
                buf->last += bytes_read;
                goto again;
            } else if (bytes_read == 0) {
                errstr = ERROR_CLOSED;
                goto err;
            } else {
                switch (errno) {
                case EAGAIN:
                    // do nothing, continue
                    continue;
                default:
                    errstr = strerror(errno);
                    goto err;
                }
            }
        }
    }

success:
    assert(buffer_size(buf) >= size);
    lua_pushlstring(L, buf->pos, size);
    buf->pos += size;
    buffer_shrink(buf);
    return 1;

err:
    assert(errstr);
    lua_pushnil(L);
    lua_pushstring(L, errstr);
    lua_pushlstring(L, buf->pos, buf->last - buf->pos);
    buf->pos = buf->last;
    buffer_shrink(buf);
    return 3;
}

static int
tcpsock_readuntil_iterator(lua_State *L)
{
    struct sockobj *s = lua_touserdata(L, lua_upvalueindex(1));
    char *errstr = NULL;
    size_t len;
    const char *pattern = lua_tolstring(L, lua_upvalueindex(2), &len);
    int state = lua_tointeger(L, lua_upvalueindex(4));
    int inclusive = lua_toboolean(L, lua_upvalueindex(3));

    if (s->buf == NULL) {
        s->buf = buffer_create(RECV_BUFSIZE);
    }
    struct buffer *buf = s->buf;

    struct timeout tm;
    timeout_init(&tm, s->sock_timeout);

again:
    do {
        int i = 0;
        int bytes = buffer_size(buf);
        if (bytes == 0) {
            break;
        }
        while (i < bytes) {
            char c = buf->pos[i];

            if (c == pattern[state]) {
                i++;
                state++;
                if (state == (int)len) {
                    /* matched */
                    buf->pos += i;
                    state = 0;
                    lua_pushinteger(L, state);
                    lua_replace(L, lua_upvalueindex(4));
                    goto matched;
                }
                continue;
            }

            if (state == 0) {
                i++;
                continue;
            }

            state = 0;
        }

        buf->pos += i;
        lua_pushinteger(L, state);
        lua_replace(L, lua_upvalueindex(4));
    } while (0);

    while (1) {
        errno = 0;
        int timeout = __waitfd(s, EVENT_READABLE, &tm);
        if (timeout == -1) {
            errstr = strerror(errno);
            goto err;
        } else if (timeout == 1) {
            errstr = ERROR_TIMEOUT;
            goto err;
        } else {
            if (buffer_available(buf) < RECV_BUFSIZE) {
                buffer_grow(buf, RECV_BUFSIZE - buffer_available(buf));
            }
            int bytes_read = recv(s->fd, buf->last, RECV_BUFSIZE, 0);
            if (bytes_read > 0) {
                buf->last += bytes_read;
                goto again;
            } else if (bytes_read == 0) {
                errstr = ERROR_CLOSED;
                goto err;
            } else {
                switch (errno) {
                case EAGAIN:
                    // do nothing, continue
                    continue;
                default:
                    errstr = strerror(errno);
                    goto err;
                }
            }
        }
    }

matched:
    if (inclusive) {
        lua_pushlstring(L, buf->start, buf->pos - buf->start);
    } else {
        lua_pushlstring(L, buf->start, buf->pos - buf->start - len);
    }
    buffer_shrink(buf);
    return 1;

err:
    assert(errstr);
    lua_pushnil(L);
    lua_pushstring(L, errstr);
    lua_pushlstring(L, buf->start, buf->pos - buf->start);
    buffer_shrink(buf);
    return 3;
}

/**
 * iterator, err = tcpsock:readuntil(pattern, inclusive?)
 */
static int
tcpsock_readuntil(lua_State *L)
{
    int n;
    n = lua_gettop(L);
    if (n != 2 && n != 3) {
        return luaL_error(L, "expecting 2 or 3 arguments (including the object), but got %d", n);
    }
    int type = lua_type(L, 2);
    if (type != LUA_TSTRING) {
        return luaL_error(L, "pattern should be string");
    }
    if (n == 3) {
        if (!lua_isboolean(L, 3)) {
            luaL_error(L, "the second argument should be boolean value");
        }
    } else {
        lua_pushboolean(L, 0);
    }
    lua_pushinteger(L, 0);

    lua_pushcclosure(L, tcpsock_readuntil_iterator, 4);
    return 1;
}

/**
 * ok, err = tcpsock:shutdown(how)
 *
 * Shut down one or both halves of the connection.
 *
 * If how is SHUT_RD, further receives are disallowed.
 * If how is SHUT_WR, further sends are disallowed.
 * If how is SHUT_RDWR, further sends and receives are disallowed.
 */
static int
tcpsock_shutdown(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    int how = luaL_checknumber(L, 2);
    int ret = shutdown(s->fd, how);
    if (ret < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

/**
 * ok, err = tcpsock:setopt(opt, value)
 */
static int
tcpsock_setopt(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    const char *opt = luaL_checkstring(L, 2);
    int flag = lua_toboolean(L, 3);
    socklen_t flagsize;
    flagsize = sizeof(flag);
    int err;
    int level;
    int optname;
    if (!strcmp(opt, OPT_TCP_KEEPALIVE)) {
        level = SOL_SOCKET;
        optname = SO_KEEPALIVE;
    } else if (!strcmp(opt, OPT_TCP_NODELAY)) {
        level = IPPROTO_TCP;
        optname = TCP_NODELAY;
    } else {
        return luaL_error(L, "unexpected option: %s", opt);
    }
    err = setsockopt(s->fd, level, optname, (void *)&flag, flagsize);
    if (err < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

/**
 * value, err = tcpsock:getopt(opt)
 */
static int
tcpsock_getopt(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    const char *opt = luaL_checkstring(L, 2);
    int flag;
    int err;
    socklen_t flagsize = sizeof(flag);
    int level;
    int optname;
    if (!strcmp(opt, OPT_TCP_KEEPALIVE)) {
        level = SOL_SOCKET;
        optname = SO_KEEPALIVE;
    } else if (!strcmp(opt, OPT_TCP_NODELAY)) {
        level = IPPROTO_TCP;
        optname = TCP_NODELAY;
    } else {
        return luaL_error(L, "unexpected option: %s", opt);
    }
    err = getsockopt(s->fd, level, optname, (void *)&flag, &flagsize);
    if (err < 0) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    lua_pushboolean(L, flag);
    return 1;
}

/**
 * addr, err = tcpsock:getpeername
 *
 * Return the address of the remote endpoint.
 */
static int
tcpsock_getpeername(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    sockaddr_t addr;
    socklen_t addrlen;
    int ret = 0, err = 0;
    if (!__getsockaddrlen(s, &addrlen)) {
        lua_pushnil(L);
        lua_pushstring(L, "unknown address family");
        return 2;
    }
    memset(&addr, 0, addrlen);
    ret = getpeername(s->fd, SAS2SA(&addr), &addrlen);
    if (ret < 0) {
        err = errno;
        lua_pushnil(L);
        lua_pushstring(L, strerror(err));
        return 2;
    }
    return __makesockaddr(L, s, SAS2SA(&addr), addrlen);
}

/**
 * addr, err = tcpsock:getsockname()
 *
 * Return the address of the local endpoint.
 */
static int
tcpsock_getsockname(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    sockaddr_t addr;
    socklen_t addrlen;
    int ret = 0, err = 0;
    if (!__getsockaddrlen(s, &addrlen)) {
        lua_pushnil(L);
        lua_pushstring(L, "unknown address family");
        return 2;
    }
    memset(&addr, 0, addrlen);
    ret = getsockname(s->fd, SAS2SA(&addr), &addrlen);
    if (ret < 0) {
        err = errno;
        lua_pushnil(L);
        lua_pushstring(L, strerror(err));
        return 2;
    }
    return __makesockaddr(L, s, SAS2SA(&addr), addrlen);
}

/**
 * ok, err = udpsock:connect(host, port)
 * ok, err = udpsock:connect("unix:/path/to/unix-domain.sock")
 *
 * Attempts to connect a UDP socket object to a remote server or to a datagram
 * unix domain socket file. Because the datagram protocol is actually
 * connection-less, this method does not really establish a "connection", but
 * only just set the name of the remote peer for subsequent read/write
 * operations.
 */
static int
udpsock_connect(lua_State * L)
{
    struct sockobj *s = getsockobj(L);
    sockaddr_t addr;
    socklen_t len;
    int n;
    n = lua_gettop(L);
    if (n != 2 && n != 3) {
        return luaL_error(L, "udpsocket:connect expecting 2 or 3 arguments"
        " (including the object itself), but seen %d", n);
    }

    if (n == 3) {
        s->sock_family = AF_INET;
    } else if (n == 2) {
        s->sock_family = AF_UNIX;
    }

    if (__getsockaddrfromarg(L, s, SAS2SA(&addr), &len)) {
        return 2;
    }

    if (__sockobj_createsocket(L, s, SOCK_DGRAM) == -1)
        return 2;

    if (__sockobj_connect(L, s, SAS2SA(&addr), len) == -1)
        return 2;

    lua_pushboolean(L, 1);
    return 1;
}

/**
 * bytes, err = udpsock:send(data)
 *
 * Sends data on the current UDP or datagram unix domain socket object.
 *
 * In case of success, it returns 1. Otherwise, it returns nil and a string
 * describing the error.
 */
static int
udpsock_send(lua_State * L)
{
}

static const luaL_Reg socketlib[] = {
    {"tcp", socket_tcp},
    {"udp", socket_udp},
    {"select", socket_select},
    {"getaddrinfo", socket_getaddrinfo},
    {NULL, NULL},
};

static const luaL_Reg sockobj_methods[] = {
    {"__gc", sockobj_close},
    {"__tostring", sockobj_tostring},
    {"close", sockobj_close},
    {"fileno", sockobj_fileno},
    {"settimeout", sockobj_settimeout},
    {"gettimeout", sockobj_gettimeout},
    {NULL, NULL},
};

static const luaL_Reg tcpsock_methods[] = {
    {"connect", tcpsock_connect},
    {"bind", tcpsock_bind},
    {"listen", tcpsock_listen},
    {"accept", tcpsock_accept},
    {"write", tcpsock_write},
    {"read", tcpsock_read},
    {"readuntil", tcpsock_readuntil},
    {"shutdown", tcpsock_shutdown},
    {"setopt", tcpsock_setopt},
    {"getopt", tcpsock_getopt},
    {"getpeername", tcpsock_getpeername},
    {"getsockname", tcpsock_getsockname},
    {NULL, NULL},
};

static const luaL_Reg udpsock_methods[] = {
    {"connect", udpsock_connect},
    {NULL, NULL},
};

int
luaopen_socket(lua_State * L)
{
    luaL_checkversion(L);
    luaL_newlib(L, socketlib);

#define ADD_NUM_CONST(name)     \
    lua_pushnumber(L, name);  \
    lua_setfield(L, -2, # name)

#define ADD_STR_CONST(name)     \
    lua_pushstring(L, name);  \
    lua_setfield(L, -2, # name)

    // OPT_* options
    ADD_STR_CONST(OPT_TCP_NODELAY);
    ADD_STR_CONST(OPT_TCP_KEEPALIVE);

    // SHUT_* sock:shutdown() parameters
    ADD_NUM_CONST(SHUT_RD);
    ADD_NUM_CONST(SHUT_WR);
    ADD_NUM_CONST(SHUT_RDWR);

    // ERROR_* some error strings, which can be used to detect errors
    ADD_STR_CONST(ERROR_TIMEOUT);
    ADD_STR_CONST(ERROR_CLOSED);
    ADD_STR_CONST(ERROR_REFUSED);

    // Create a metatable for tcp socket userdata.
    luaL_newmetatable(L, TCPSOCK_TYPENAME);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");     /* metable.__index = metatable */
    luaL_setfuncs(L, sockobj_methods, 0);
    luaL_setfuncs(L, tcpsock_methods, 0);
    lua_pop(L, 1);

    // Create a metatable for udp socket userdata.
    luaL_newmetatable(L, UDPSOCK_TYPENAME);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");     /* metable.__index = metatable */
    luaL_setfuncs(L, sockobj_methods, 0);
    luaL_setfuncs(L, udpsock_methods, 0);
    lua_pop(L, 1);

    // install a handler to ignore sigpipe or it will crash us
    signal(SIGPIPE, SIG_IGN);

    return 1;
}
