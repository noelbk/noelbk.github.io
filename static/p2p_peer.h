// p2p_peer.h - main entry for p2p desktop daemon 
// Noel Burton-Krahn <noel@burton-krahn.com>

#ifndef P2P_PEER_H_INCLUDED
#define P2P_PEER_H_INCLUDED

#include "p2p_msg.h"
#include "p2p_ctl_peer.h"
#include "p2p_router.h"
#include "ethertap.h"
#include "p2p_peer_const.h"
#include "sqlbk.h"

#include "bklib/mstime.h"
#include "bklib/sock.h"
#include "bklib/array.h"
#include "bklib/hash.h"
#include "bklib/fdselect.h"
#include "bklib/rand.h"
#include "bklib/state.h"
#include "bklib/debug.h"
#include "bklib/cryptbk.h"
#include "bklib/cfg.h"
#include "bklib/dns.h"
#include "bklib/pool.h"
#include "bklib/ipconntrack.h"
#include "bklib/proc.h"

typedef cryptbk_t p2p_crypt_t;

enum p2p_peer_server_state_t {
    P2P_PEER_SERVER_HALT=0
    ,P2P_PEER_SERVER_HALT_DELAY
    ,P2P_PEER_SERVER_CLOSE
    ,P2P_PEER_SERVER_DELETE
    ,P2P_PEER_SERVER_RECONNECT_DELAY
    ,P2P_PEER_SERVER_INIT
    ,P2P_PEER_SERVER_DNS
    ,P2P_PEER_SERVER_DNS_FAILED
    ,P2P_PEER_SERVER_DNS_OK
    ,P2P_PEER_SERVER_HOST
    ,P2P_PEER_SERVER_HOST_FAILED
    ,P2P_PEER_SERVER_HOST_OK
    ,P2P_PEER_SERVER_CERT
    ,P2P_PEER_SERVER_CERT_FAILED
    ,P2P_PEER_SERVER_CERT_OK
};

typedef struct p2p_client_s p2p_client_t;
typedef struct p2p_peer_server_s p2p_peer_server_t;

/* p2p_peer_router_recv_func gets a pointer to this struct withing a
   client or server, so it can tell the difference.  I could save some
   code by making clients and servers use the same structure.  They're
   very similar.  But, for now I'll keep the legacy setup. */
typedef struct p2p_peer_client_or_server_t {
    p2p_client_t      *client;
    p2p_peer_server_t *server;
} p2p_peer_client_or_server_t;


/* every client and peer is associated with a server which brokers
   connections between them.  Every server needs to be resolved by DNS
   and I need its public cert. Several clients can poll the same
   server record with p2p_peer_server_poll() */
struct p2p_peer_server_s {
    // a back pointer used in p2p_peer_router_recv
    p2p_peer_client_or_server_t client_or_server;

    pool_t           *pool;
    state_t          state;
    int              refcount;

    char             ui_msg[4096];
    p2p_ctl_peer_ui_state_t ui_state;
    int              ui_code;
    int              ui_prog;

    char            *hostname;
    unsigned short   udp_port; 
    unsigned short   tcp_port; 
    char            *hostname_port; /* used as the hash key */
    p2p_msg_t        info_msg;
    mstime_t         time_expire;
    mstime_t         time_offset;
    p2p_sockaddr_t   dns_addr;
    p2p_router_host_t *host; /* the host I use to connect and send */
};

/** One record for each connecting client.
 */
struct p2p_client_s {
    pool_t               *pool;

    // a back pointer used in p2p_peer_router_recv
    p2p_peer_client_or_server_t client_or_server;

    /* this client's server. server->host->src_id gives me a unique id
       at the client */
    p2p_peer_server_t    *server;

    p2p_seskey_t    peer_seskey;
    char           *peer_username; /* the username/password I send to the client */
    char           *peer_password; 
    
    int             started_here; /* true iff I called the client */

    char           *client_hostname; /* the name of the remote host */
    char           *client_username; /* the username/password the client sent to me */
    char           *client_password; 
    int            client_opts; /* from P2P_PEER_OPT_* */
    mstime_t       client_expire;

    p2p_keyval_t   *client_extra;
    p2p_keyval_t   *client_search;

    /* my current state for my external UI controls */
    char                  ui_msg[4096];
    p2p_ctl_peer_ui_state_t                  ui_state;
    int                   ui_code;
    int                   ui_prog;
    p2p_keyval_t         *ui_extra;

    p2p_version_t   client_version;

    p2p_router_host_t *host; /* the router_host I use to connect and send */

    p2p_sockaddr_t  client_pseudo_ethaddr;  /**< ether addr on my iface */
    p2p_sockaddr_t  client_pseudo_ipaddr;   /**< ip addr on my iface */

    /* connecting or connected */
    p2p_msg_t       client_connect_msg; 
    state_t         client_connect_state;
    int             client_connect_accepted; /* true iff my CLIENT_CONNECT was accepted */
    p2p_seskey_t    client_connect_seskey;
    array_t         client_dns_reply;   /* DNS queries waiting if client exists */
    mstime_t        client_ping_sent;       /**< the last time I pinged the client */
    mstime_t        client_ping_recv;       /**< the last time I pinged the client */
    mstime_t        client_ping_rtt;        /**< the last time I pinged the client */

    hash_t          pkt_mangle;
    IpConnTrack     pkt_conntrack;
};

/** the main peer record. */
struct p2p_peer_s {
    pool_t               *pool; /* holds malloced and strdup'ed data */
    sql_t                *db;
    char                 *db_path;
    cfg_t                *cfg;
    int                  opts; /* current P2P_CTL_PEER_OPT_* options */
    int                  opts_argv; /* flag values set by argv */
    int                  opts_argv_mask; /* flag positions set by argv */

    mstime_t             poll_cfg_time_last;
    mstime_t             poll_cfg_interval;
    mstime_t             poll_update_time_last;
    mstime_t             poll_update_interval;
    mstime_t             cfg_dir_time; /* mtime of /etc/p2p_peer */

    int                  exit; /* set to true to exit */
    int                  exit_code; /* code to send to server, clients and controls */
    char                *exit_text;
    
    int                  debug_level; /* my debugging level */
    int                  in_debug; /* don't recurse debug messages */
    debug_func_t         debug_func;
    void                *debug_arg;
    char                *log_prefix;
    void                *debug_log_arg; /* to pass to debug_func_log */
    
    /* the server I logged in to */
    p2p_peer_server_t    *server;

    /* logging in */
    char                 *server_hostname;
    char                 *peer_hostname;
    char                 *peer_hostname_old;
    char                 *peer_password;
    char                 *peer_password_old; /**< my last password while changing */
    int                   peer_newhost;  /**< true iff the peer is trying to register as a new host */
    int                   closing;

    p2p_router_t         *router;

    p2p_version_t         peer_version;
    p2p_keyval_t         *peer_search; /* searchable properties */
    p2p_keyval_t         *peer_extra;

    cryptbk_t            *peer_crypt; /* cert for messages */
    cryptbk_t            *peer_crypt_secret; /* cert for encrypting saved passwords */
    mstime_t              peer_expire; /* when my certificate expires */

    /* login state machine */
    int                   login_pending; // true iff I'm waiting for login to complete
    state_t               login_state;
    p2p_msg_t             login_server_msg;
    mstime_t              login_time;
    p2p_seskey_t          login_seskey;
    mstime_t              server_ping_sent;
    mstime_t              server_ping_recv;
    mstime_t              server_ping_rtt;

    /* udp socket to accept packets from server and clients */
    unsigned short        udp_port;
    unsigned short        tcp_port;

    /* servers for myself and my clients, hashed by hostname:port */
    hash_t                server_hash_hostname_port; 

    /* my clients, hashed by hostname, socket, pseudo ether, and ip addresses */ 
    hash_t                client_hash_client_hostname; 
    hash_t                client_hash_pseudo_ipaddr; 
    hash_t                client_hash_pseudo_ethaddr; 

    /* local TCP socket for control */
    array_t               ctl_list;
    int                   ctl_port;
    sock_t                ctl_sock;
    char                  *ctl_username; /* auth required to connect */
    char                  *ctl_password;

    /* my current state for my external UI controls */
    char                  ui_msg[4096];
    p2p_ctl_peer_ui_state_t                   ui_state;
    int                   ui_code;
    int                   ui_prog;
    
    /* my pseudo-network, the next client's ip and eth address */
    int                   no_ethertap; // true iff I'm not using an ethertap
    ethertap_t            ethertap;

    p2p_ctl_peer_netcfg_t  netcfg;
    p2p_ctl_peer_conn_t    *netcfg_conn;

    int                   min_mtu;
    unsigned char         peer_pseudo_ethaddr[6]; 
    unsigned long         peer_pseudo_ipaddr; 
    unsigned long         peer_pseudo_netmask; 

    unsigned long         client_pseudo_ipaddr_start; 
    unsigned long         client_pseudo_ipaddr_max; 

    unsigned long         dns_pseudo_ipaddr; 
    unsigned char         dns_pseudo_ethaddr[6]; 
    procid_t              peer_dns_proxy_cleanup_pid;


    /* multiplex all my sockets */
    fdselect_t            *fdselect;
    int                   in_select;
    mstime_t              mstime;

    /* proxying DNS requests to real DNS servers */
    array_t               dns_proxy_addrs;
    hash_t                dns_proxy_hash_id;
    sock_t                dns_proxy_sock;

};
typedef struct p2p_peer_s p2p_peer_t;

/* millseconds to poll all active clients */
#define P2P_PEER_POLL_INTERVAL 200

/* interval in seconds to check for network changes */
#define P2P_PEER_NETCFG_POLL_INTERVAL 15

/* seconds to poll /etc/p2p_peer for changes */
#define P2P_PEER_POLL_CFG_INTERVAL 2

/* seconds to check for updates */
#define P2P_PEER_POLL_UPDATE_INTERVAL (24*3600)

typedef enum {
    P2P_PEER_CONNECT_NONE=0,
    P2P_PEER_CONNECT_INIT,
    P2P_PEER_CONNECT_SERVER_RESOLVE,  // resolve server's IP and get cert
    P2P_PEER_CONNECT_SERVER,          // send client_connect to peer's sever
    P2P_PEER_CONNECT_SERVER_FAIL,
    P2P_PEER_CONNECT_SERVER_ACCEPTED,
    P2P_PEER_CONNECT_SERVER_REJECTED, 
    P2P_PEER_CONNECT_HOST,            // wait for p2p_router_host_connect 
    P2P_PEER_CONNECT_HOST_FAIL,
    P2P_PEER_CONNECT_HOST_OK,
    P2P_PEER_CONNECT_PEER,            // send client_connect to my peer
    P2P_PEER_CONNECT_PEER_FAIL,
    P2P_PEER_CONNECT_PEER_REJECTED,
    P2P_PEER_CONNECT_PEER_ACCEPTED,
    P2P_PEER_CONNECT_CTL,             // asking my control to accept client
    P2P_PEER_CONNECT_CTL_TIMEOUT,
    P2P_PEER_CONNECT_CTL_REJECTED,
    P2P_PEER_CONNECT_ACCEPT,          // accepting client_connect from client
    P2P_PEER_CONNECT_ACCEPT_FAILED,
    P2P_PEER_CONNECT_CONFIRMED,       // client or peer confirmed
    P2P_PEER_CONNECT_CLOSING,         // close and don't reopen for a little while
    P2P_PEER_CONNECT_CLOSE,
    P2P_PEER_CONNECT_DELETE
} p2p_peer_connect_state_t;

char *
p2p_peer_connect_state_str(int connnect_state);

state_table_t *p2p_peer_connect_state_table;

typedef enum {
    P2P_PEER_LOGIN_NONE=0
    ,P2P_PEER_LOGIN_INIT // start login process
    ,P2P_PEER_LOGIN_HALT // login process halted, waiting for input
    ,P2P_PEER_LOGIN_GET_CONFIG // get my hostname, cert, pass, etc
    ,P2P_PEER_LOGIN_RESOLVE_SERVER // find my server's address
    ,P2P_PEER_LOGIN_RESOLVE_SERVER_FAILED // failed to find my server's address
    ,P2P_PEER_LOGIN_SEND_LOGIN // send my login to my server 
    ,P2P_PEER_LOGIN_SEND_LOGIN_FAILED // server failed to reply
    ,P2P_PEER_LOGIN_SERVER_ACCEPTED // server accepted my login
} p2p_peer_login_state_t;
char *p2p_peer_login_state_str(state_t *state);

state_table_t *p2p_peer_login_state_table;

/** initialize a p2p_peer_t.  
    \param peer an uninitialized structure
    \return 0 if successful.
 */
int
p2p_peer_init(p2p_peer_t *peer, fdselect_t *fdselect);

int
p2p_peer_poll(p2p_peer_t *peer);

int
p2p_peer_fini(p2p_peer_t *peer);

int
p2p_peer_cfg_load(p2p_peer_t *peer);

int
p2p_peer_cfg_save(p2p_peer_t *peer);

int
p2p_peer_argv(p2p_peer_t *peer, int *argc, char **argv);

int
p2p_peer_run(p2p_peer_t *peer);

int
p2p_peer_poll_pre_select(p2p_peer_t *peer);

int
p2p_peer_shutdown(p2p_peer_t *peer);

/* p2p_peer_db.c */

/* assumes peer->db_path is set */
int
p2p_peer_db_open(p2p_peer_t *peer);

int
p2p_peer_db_close(p2p_peer_t *peer);

/* called by p2p_peer_db_openm, assumes peer->db is set */
int
p2p_peer_db_create(p2p_peer_t *peer);

int
p2p_peer_db_set_client_login(p2p_peer_t *peer, 
			     char *username, char *password, mstime_t expire);

int
p2p_peer_db_set_app_info(p2p_peer_t *peer, 
			  p2p_ctl_peer_app_info_t *m);

/* returns:
   >0 => rejected (P2P_ERR_LOGIN_EXPIRED or P2P_ERR_LOGIN_BAD_PASSWORD)
    0 => accepted
   <0 => not accepted ot rejected
*/
int
p2p_peer_db_check_client_login(p2p_peer_t *peer,
			       char *hostname, 
			       char *username, 
			       char *password);

int
p2p_peer_db_set_client_connect(p2p_peer_t *peer, 
			       char *hostname, char *username, char *password,
			       mstime_t expire);

int
p2p_peer_db_get_client_connect(p2p_peer_t *peer, char *hostname, 
			       char **username, char **password,
			       pool_t *pool);

int
p2p_peer_db_used_client_connect(p2p_peer_t *peer, 
				char *hostname,
				char *username,
				char *password
				);

/* p2p_peer.c */

int
p2p_peer_secret_crypt(p2p_peer_t *peer,
		      char *src, int srclen, 
		      char *dst, int dstlen, 
		      char *tmp, int tmplen,
		      int mode);

int
p2p_peer_connect_poll(p2p_peer_t *peer, p2p_client_t *client);

int
p2p_peer_login_poll(p2p_peer_t *peer);

/* returns 1 iff login was accepted by server */
int
p2p_peer_login_accepted(p2p_peer_t *peer);

p2p_client_t*
p2p_peer_client_new(p2p_peer_t *peer);

int
p2p_peer_client_delete(p2p_peer_t *peer, p2p_client_t *client);

p2p_client_t*
p2p_peer_client_get_client_hostname(p2p_peer_t *peer, char *name, int create);

int
p2p_peer_client_set_client_hostname(p2p_peer_t *peer, p2p_client_t *client, char *name);

/* find a client by p2p_router_host's src_id */
p2p_client_t*
p2p_peer_client_get_id(p2p_peer_t *peer, p2p_sockaddr_t *addr, int create);

/* find a client by local faked IP address */
p2p_client_t*
p2p_peer_client_get_ipaddr(p2p_peer_t *peer, char *ipaddr);

p2p_client_t*
p2p_peer_client_get_ethaddr(p2p_peer_t *peer, char *ethaddr);

int
p2p_peer_client_connect_start(p2p_peer_t *peer, p2p_client_t *client, int force);

int
p2p_peer_client_connect_accept(p2p_peer_t *peer, p2p_client_t *client);

int
p2p_peer_client_reconnect(p2p_peer_t *peer, p2p_client_t *client);

int
p2p_peer_client_connect_close(p2p_peer_t *peer, p2p_client_t *client);

int
p2p_peer_client_connected(p2p_peer_t *peer, p2p_client_t *client);

//--------------------------------------------------------------------
/* p2p_peer_server.c */

/* find a server for a client's hostname */
p2p_peer_server_t*
p2p_peer_server_get_hostname(p2p_peer_t *peer, char *hostname, int create);

p2p_peer_server_t*
p2p_peer_server_get_sockaddr(p2p_peer_t *peer, p2p_sockaddr_t *addr);

int
p2p_peer_server_init(p2p_peer_t *peer);

int
p2p_peer_server_fini(p2p_peer_t *peer);

/* return 1 if the server is connected, returns -1 if failed to
   connect, otherwise keep connecting and returns 0 */
int
p2p_peer_server_connect_poll(p2p_peer_t *peer, p2p_peer_server_t *server);
/* poll the server for connectedness.  returns one of the P2P_PEER_SERVER_XXX enums */
int 
p2p_peer_server_poll(p2p_peer_t *peer, p2p_peer_server_t *server);

int
p2p_peer_poll_all_servers(p2p_peer_t *peer);

int
p2p_peer_server_resolved(p2p_peer_t *peer, p2p_peer_server_t *server);

/* handle a P2P_MSG_SERVER_INFO */
int
p2p_peer_recv_server_info(p2p_peer_t *peer, p2p_msg *msg);

int 
p2p_peer_server_log(p2p_peer_t *peer, p2p_peer_server_t *server, char *fmt, ...);

int 
p2p_peer_log(p2p_peer_t *peer, char *fmt, ...);

//--------------------------------------------------------------------
int
p2p_peer_ethertap_open(p2p_peer_t *peer);

void
p2p_peer_ethertap_poll(fdselect_t *sel, fd_t fd, int events, void *arg);

/* p2p_peer_dns.c */

typedef int 
(*p2p_peer_dns_reply_func_t)(p2p_peer_t *peer, dns_t *dns, p2p_sockaddr_t *addr, void *arg);

int
p2p_peer_dns_proxy_init(p2p_peer_t *peer);

int
p2p_peer_dns_proxy_resolv_conf(p2p_peer_t *peer);

int
p2p_peer_dns_proxy_fini(p2p_peer_t *peer);

/* handle dns queries in a raw ethernet buffer */
int
p2p_peer_dns_ether(p2p_peer_t *peer, struct netpkt *pkt);

// expire old requests
int
p2p_peer_dns_proxy_poll(p2p_peer_t *peer);

// send a request for a hostname
int
p2p_peer_dns_proxy_resolve(p2p_peer_t *peer, char *hostname, 
			   p2p_peer_dns_reply_func_t func, void *arg);

// forward an aribitrary dns request
int
p2p_peer_dns_proxy_send(p2p_peer_t *peer, dns_t *dns,
			p2p_peer_dns_reply_func_t func, void *arg);


int p2p_peer_debug(char *buf, void *arg);

int p2p_peer_ctl_init(p2p_peer_t *peer);

int p2p_peer_ctl_fini(p2p_peer_t *peer);

typedef int (*p2p_peer_client_foreach_func)(p2p_peer_t *peer, p2p_client_t *client, void *arg);

int p2p_peer_client_foreach(p2p_peer_t *peer, p2p_peer_client_foreach_func func, void *arg);

int
p2p_peer_client_close(p2p_peer_t *peer, p2p_client_t *client);

int
p2p_peer_client_foreach_close(p2p_peer_t *peer, p2p_client_t *client, void *arg);

// p2p_peer_pkt.c
int p2p_peer_pkt_mangle(p2p_peer_t *peer, p2p_client_t *client, 
			struct netpkt *pkt, int from_client);

int
p2p_peer_pkt_check_acl(p2p_peer_t *peer, p2p_client_t *client, 
		       struct netpkt *pkt, int from_client,
		       char *portbuf, int portbuflen);

int
p2p_peer_pkt_mangle_init(p2p_peer_t *peer, p2p_client_t *client);

int
p2p_peer_pkt_mangle_fini(p2p_peer_t *peer, p2p_client_t *client);


// p2p_peer_ctl.c
int
p2p_peer_ui(p2p_peer_t *peer, p2p_ctl_peer_ui_state_t state, int progress, char *msg, ...);


// p2p_peer_cert.c
int
p2p_peer_cert_gen_req(p2p_peer_t *peer);

int
p2p_peer_ctl_bcast(p2p_peer_t *peer, p2p_ctl_peer_t *msg);

int
p2p_peer_ctl_client_ui(p2p_peer_t *peer, p2p_client_t *client,
		       p2p_ctl_peer_ui_state_t state, int code, int prog, char *msg);

int
p2p_peer_server_ui(p2p_peer_t *peer, p2p_peer_server_t *server, 
		   p2p_ctl_peer_ui_state_t ui_state, int ui_code, int ui_prog,
		   char *fmt, ...);

#define P2P_PEER_CERT_BITS 2048
#define P2P_PEER_CERT_DN   "CN=newhost"

/* p2p_peer_router.h */

/* pack up a p2p_msg and send it to a host (either client or server) */
int
p2p_peer_msg_send_host(p2p_router_host_t *host, p2p_msg *msg);

int
p2p_peer_router_recv(void *arg, p2p_router_recv_ctx_t *ctx);

int
p2p_peer_recv(p2p_peer_t *peer, p2p_router_recv_ctx_t *ctx);

int p2p_peer_netcfg_init(p2p_peer_t *peer);
int p2p_peer_netcfg_fini(p2p_peer_t *peer);

int
p2p_peer_server_reconnect_all(p2p_peer_t *peer);

#endif // P2P_PEER_H_INCLUDED
