// p2p_peer.h - main entry for p2p desktop daemon 
// Noel Burton-Krahn <noel@burton-krahn.com>

#include <sys/stat.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "p2p_peer.h"
#include "opt.h"
#include "debug.h"
#include "debug_stdio.h"
#include "dir.h"
#include "cfg_crypt.h"
#include "ifcfg.h"
#include "defutil.h"

#include "bklib/sig.h"
#include "bklib/env.h"
#include "bklib/service.h"
#include "bklib/mstime.h"

#include "p2p_hostname.h"
#include "p2p_version.h"

// p2p peers are named "hostname.p2p[-port].server" or
// "hostname.p2p[-port].1.2.3.4.p2p".  This resolves the server and port
// parts of the hostname.
//

/** initialize a p2p_peer_t.  
    \param peer an uninitialized structure
    \return 0 if successful.
 */
int
p2p_peer_init(p2p_peer_t *peer, fdselect_t *fdselect) {
    int i, err=-1;

    do {
	memset(peer, 0, sizeof(*peer));

	peer->peer_version.major = P2PVPN_VERSION_MAJOR;
	peer->peer_version.minor = P2PVPN_VERSION_MINOR;

	peer->poll_cfg_interval = P2P_PEER_POLL_CFG_INTERVAL;
	peer->poll_update_interval = P2P_PEER_POLL_UPDATE_INTERVAL;
	
	peer->pool = pool_new();
	assertb(peer->pool);

	i = hash_init(&peer->client_hash_client_hostname, 
		      hash_hash_str_nocase, hash_cmp_str_nocase,
		      0, 0, 1);
	assertb(i>=0);
	pool_add(peer->pool, &peer->client_hash_client_hostname, pool_free_hash, 0);

	i = hash_init(&peer->client_hash_pseudo_ethaddr, 
		      p2p_sockaddr_hash, p2p_sockaddr_cmp,
		      0, 0, 1);
	assertb(i>=0);
	pool_add(peer->pool, &peer->client_hash_pseudo_ethaddr, pool_free_hash, 0);

	i = hash_init(&peer->client_hash_pseudo_ipaddr, 
		      p2p_sockaddr_hash, p2p_sockaddr_cmp,
		      0, 0, 1);
	assertb(i>=0);
	pool_add(peer->pool, &peer->client_hash_pseudo_ipaddr, pool_free_hash, 0);

	i = state_init(&peer->login_state, p2p_peer_login_state_table);
	assertb(i>=0);

	peer->fdselect = fdselect;

	peer->server_hostname = "natnix.com";
	peer->log_prefix = pool_strdup(peer->pool, "log/peer");
	
	peer->dns_proxy_sock = -1;
	peer->ctl_sock = -1;

	err = 0;
    } while(0);
    return err;
}

int
p2p_peer_fini(p2p_peer_t *peer) {
    int err=-1;

    do {
	p2p_peer_netcfg_fini(peer);

	pool_delete(peer->pool);
	err = 0;
    } while(0);
    return err;
}

/* load options, but don't override options set from argv */
int
p2p_peer_cfg_load(p2p_peer_t *peer) {
    int i, n, x, err=-1;
    char *p, buf[4096], buf1[4096], path[4096], newpath[4096];
    p2p_keyval_t *kv, *next;
    cfg_enum_t *e;
    cryptbk_info_t info;

    info.subject = 0;
    do {
	if( !peer->cfg ) {
	    peer->cfg = cfg_new(P2P_PEER_CFG_NAME);
	    assertb(peer->cfg);
	}
	
	/* server's hostname */
	i = cfg_get(peer->cfg, "server", buf, sizeof(buf));
	if( i > 0 ) {
	    pool_copy_str(peer->pool, &peer->server_hostname, str_trim(buf));
	}
	
	/* secret certificate for passwords */
	if( peer->peer_crypt_secret ) {
	    cryptbk_delete(peer->peer_crypt_secret);
	    peer->peer_crypt_secret = 0;
	}
	peer->peer_crypt_secret = cfg_load_crypt(peer->cfg, "secret", 
						 "secret", 0, 
						 2048, 365*24*3600);
	assertb(peer->peer_crypt_secret);

	/* my certificate for my hostname */
	if( peer->peer_crypt ) {
	    cryptbk_delete(peer->peer_crypt);
	    peer->peer_crypt = 0;
	    if( peer->router ) {
		p2p_router_crypt_set(peer->router, 0, 0);
	    }
	}

	i = cfg_get(peer->cfg, "hostname", buf, sizeof(buf));
	if( i>0 ) {
	    p = str_trim(buf);
	    if( peer->peer_hostname && strcmp(p, peer->peer_hostname) != 0 ) {
		pool_copy_str(peer->pool, &peer->peer_hostname_old, peer->peer_hostname);
	    }
	    pool_copy_str(peer->pool, &peer->peer_hostname, p);
	}

	p2p_peer_ui(peer, P2P_PEER_UI_CERT, 0,
		    "Generating host certificate");

	if( peer->peer_hostname && *peer->peer_hostname ) {
	    
	    // load or generate a cert
	    peer->peer_crypt = cfg_load_crypt(peer->cfg, "certificate", 
					      peer->peer_hostname, 0, 
					      2048, 365*24*3600);
	    assertb(peer->peer_crypt);

	    // extract and save the server name part
	    i = p2p_hostname_server(peer->peer_hostname, buf, sizeof(buf), &i);
	    if( i == 0 ) {
		pool_copy_str(peer->pool, &peer->server_hostname, str_trim(buf));
	    }

	}
	else {
	    // load or generate a cert
	    peer->peer_crypt = cfg_load_crypt(peer->cfg, "certificate", 
					      "anonymous", 0, 
					      2048, 365*24*3600);
	    assertb(peer->peer_crypt);
	}

	/* remember to give my router my new crypt */
	if( peer->router ) {
	    i = p2p_router_crypt_set(peer->router, peer->peer_crypt, 0);
	    assertb(i>=0);
	}

	if( !peer->peer_hostname ) {
	    peer->peer_hostname = "";
	}

	/* kludge - always register as new */
	peer->peer_newhost = 1;

	/* try to load my password */
	i = cfg_get(peer->cfg, "password_plain", buf, sizeof(buf));
	if( i > 0 ) {
	    /* read a new plain password */
	    if( peer->peer_password ) {
		/* save the old password to change it */
		pool_copy_str(peer->pool, &peer->peer_password_old, peer->peer_password);
	    }
	    pool_copy_str(peer->pool, &peer->peer_password, str_trim(buf));

	    /* delete the plain password file */
	    cfg_dir(peer->cfg, CFG_DIR_GLOBAL, buf, sizeof(buf));
	    snprintf(newpath, sizeof(newpath), "%s/password_plain", buf);
	    unlink(newpath);
	}
	else if( !peer->peer_password 
		 && (i = cfg_get(peer->cfg, "password", buf, sizeof(buf)))>0 ) {
	    i = p2p_peer_secret_crypt(peer, 
				      buf, i,
				      buf, sizeof(buf),
				      buf1, sizeof(buf1),
				      CRYPTBK_MODE_DEC);
	    if( i>=0 ) {
		pool_copy_str(peer->pool, &peer->peer_password, buf);
	    }
	}

	/* make search directory */
	cfg_dir(peer->cfg, CFG_DIR_GLOBAL, path, sizeof(path));
	snprintf(newpath, sizeof(newpath), "%s/search.d", path);
	dir_mkpath(path, 1);

	/* move old-style search keys to new search directory */
	cfg_dir(peer->cfg, CFG_DIR_GLOBAL, path, sizeof(path));
	i = cfg_get(peer->cfg, "search", buf, sizeof(buf));
	if( i > 0 ) {
	    FILE *f = 0;
	    char valbuf[1024];
	    char keybuf[1024];
	    
	    p = buf;
	    n = i;
	    while( p2p_extra_list_next(&p, keybuf, sizeof(keybuf)) ) {
		i = cfg_get(peer->cfg, keybuf, valbuf, sizeof(valbuf));
		if( i <= 0 ) { continue; }

		snprintf(newpath, sizeof(newpath), "%s/search.d/%s"
			 , path, keybuf);
		f = fopen(newpath, "w");
		if( f ) {
		    fwrite(valbuf, 1, i, f);
		    fclose(f);
		}
		snprintf(newpath, sizeof(newpath), "%s/%s", path, keybuf);
		unlink(newpath);
	    }
	    snprintf(newpath, sizeof(newpath), "%s/%s", path, "search");
	    unlink(newpath);
	}

	/* free old search keys */
	for(kv=peer->peer_search; kv; kv=next) {
	    next = kv->next;
	    pool_free(peer->pool, kv->key);
	    pool_free(peer->pool, kv->val);
	    pool_free(peer->pool, kv);
	}
	peer->peer_search = 0;

	/* read search keys */
	e = cfg_enum_open(peer->cfg, CFG_DIR_GLOBAL, "search.d");
	while(e) {
	    FILE *f = 0;
	    char valbuf[1024];
	    char keybuf[1024];
	    p2p_keyval_t kvbuf;
	    
	    p = cfg_enum_read(e, path, sizeof(path));
	    if( !p ) break;
	    f = fopen(path, "r");
	    if( !f ) continue;
	    dir_basename(path, keybuf, sizeof(keybuf));
	    i = fread(valbuf, 1, sizeof(valbuf)-1, f);
	    fclose(f);
	    if( i < 0 ) i = 0;
	    valbuf[i] = 0;
	    p = str_trim(valbuf);
	    if( !p || !*p ) continue;
	    memset(&kvbuf, 0, sizeof(kvbuf));
	    kvbuf.key = keybuf;
	    kvbuf.val = valbuf;
	    p2p_keyval_merge(&peer->peer_search, &kvbuf, peer->pool, 1);
	}
	if( e ) {
	    cfg_enum_close(e);
	}

	/* load opts, but don't override opts set by argv */
	i = cfg_get_int(peer->cfg, "opts", &x);
	if( i>=0 ) {
	    x &= ~peer->opts_argv_mask;
	    x |= peer->opts_argv;
	    peer->opts = x;
	}


	err = 0;
    } while(0);
    if( info.subject ) {
	cryptbk_info_free(&info);
    }

    return err;
}

int
p2p_peer_cfg_save(p2p_peer_t *peer) {
    int i, n, err=-1;
    char *p, buf[4096], buf2[4096], path[4096];
    p2p_keyval_t *kv;
    struct stat st;

    do {
	assertb(peer->cfg);
	
	/* server's hostname */
	i = cfg_put(peer->cfg, "server", peer->server_hostname, -1, CFG_PUT_GLOBAL);

	/* hostname */
	i = cfg_put(peer->cfg, "hostname", peer->peer_hostname, -1, CFG_PUT_GLOBAL);
	
	/* secret certificate */
	if( peer->peer_crypt_secret ) {
	    i = cryptbk_pack(peer->peer_crypt_secret, buf, sizeof(buf), 0,
			     CRYPTBK_MODE_ENC
			     | CRYPTBK_MODE_TEXT
			     | CRYPTBK_MODE_PUBKEY
			     | CRYPTBK_MODE_PRIVKEY
			     );
	    assertb(i>0);
	    i = cfg_put(peer->cfg, "secret", buf, -1, CFG_PUT_GLOBAL);
	    assertb(i>0);
	}

	/* certificate */
	if( peer->peer_crypt ) {
	    i = cryptbk_pack(peer->peer_crypt, buf, sizeof(buf), 0,
			     CRYPTBK_MODE_ENC
			     | CRYPTBK_MODE_TEXT
			     | CRYPTBK_MODE_PUBKEY
			     | CRYPTBK_MODE_PRIVKEY
			     );
	    assertb(i>0);
	    i = cfg_put(peer->cfg, "certificate", buf, -1, CFG_PUT_GLOBAL);
	    assertb(i>0);
	}
	
	/* opts */
	i = cfg_put_int(peer->cfg, "opts", peer->opts, CFG_PUT_GLOBAL);
	assertb(i>=0);

	/* password (encrypted) */
	if( peer->peer_crypt && peer->peer_password ) {
	    if( peer->opts & P2P_PEER_OPT_SAVE_PASSWORD ) {
		i = p2p_peer_secret_crypt(peer, 
					  peer->peer_password, strlen(peer->peer_password),
					  buf, sizeof(buf),
					  buf2, sizeof(buf2),
					  CRYPTBK_MODE_ENC);
		assertb(i>0);
		i = cfg_put(peer->cfg, "password", buf, -1, CFG_PUT_GLOBAL);
		assertb(i>0);
	    }
	}
	
	/* peer_search */
	p = buf;
	n = sizeof(buf);
	cfg_dir(peer->cfg, CFG_DIR_GLOBAL, path, sizeof(path));
	for(kv=peer->peer_search; kv; kv=kv->next) {
	    char newpath[4096];
	    FILE *f;
	    
	    snprintf(newpath, sizeof(newpath),
		     "%s/search.d/%s"
		     , path, kv->key);
	    dir_mkpath(newpath, 0);
	    f = fopen(newpath, "w");
	    if( f ) {
		fwrite(kv->val, 1, strlen(kv->val), f);
		fclose(f);
	    }
	}
	assertb(!kv);

	/* remember the mtime */
	cfg_dir(peer->cfg, CFG_DIR_GLOBAL, path, sizeof(path));
	i = stat(path, &st);
	if( i==0 ) {
	    peer->cfg_dir_time = st.st_mtime;
	}

	err = 0;
    } while(0);
    return err;
}

typedef struct {
    p2p_peer_t *peer;
    p2p_sockaddr_t *addr;
    int err;
} p2p_peer_sockerr_t;

int
p2p_peer_client_foreach_connect_poll(p2p_peer_t *peer, p2p_client_t *client, void *arg) {
    p2p_peer_connect_poll(peer, client);
    return 0;
}


/* look through all clients and servers for the address that failed.
   If it's a server, start reconnecting to the server.  
   If it's a client's local address, ignore the local address.  
   If it's a client's connected address, start reconnecting to the client.
 */

int
p2p_peer_update_func(update_t *up) {
    p2p_peer_t *peer = (p2p_peer_t*)up->farg;
    int ret = 0;

    switch(up->state) {
    case UPDATE_STATE_ASK: {
	if( !up->cmp_newer ) {
	    ret = 1;
	}
	debug(DEBUG_INFO, 
	      ("p2p_peer_update: updating old_ver=%s new_ver=%s\n"
	       ,up->old_ver, up->new_ver));
	break;
    }

    case UPDATE_STATE_DOWNLOAD: {
	debug(DEBUG_INFO, 
	      ("p2p_peer_update: downloading url=%s to new_path=%s\n"
	       ,up->new_url, up->new_path));
	break;
    }

    case UPDATE_STATE_PROGRESS: {
	break;
    }

    case UPDATE_STATE_INSTALL: {
	debug(DEBUG_INFO, 
	      ("p2p_peer_update: installing new_path=%s\n"
	       ,up->new_path));
	break;
    }

    case UPDATE_STATE_UPDATED: {
	debug(DEBUG_INFO, 
	      ("p2p_peer_update: updated\n"
	       ));
	p2p_peer_shutdown(peer);
	break;
    }
    default: 
	break;
    }
    
    return ret;
    
}

int
p2p_peer_update(p2p_peer_t *peer) {
    int i, update_auto=0;
    char buf[4096];
    
    do {
	i = cfg_get_int(peer->cfg, "update_auto", &update_auto);
	if( i<0 || !update_auto ) break;
	i = cfg_get(peer->cfg, "version", buf, sizeof(buf));
	if( i<=0 ) break;
	update(buf, p2p_peer_update_func, peer);
    } while(0);
    return 0;
}

int
p2p_peer_poll(p2p_peer_t *peer) {
    int i;
    do {
	peer->mstime = mstime();

	if( peer->mstime - peer->poll_update_time_last 
	    > peer->poll_update_interval ) {
	    p2p_peer_update(peer);
	    peer->poll_update_time_last = peer->mstime;
	}
	
	if( peer->mstime - peer->poll_cfg_time_last 
	    > peer->poll_cfg_interval ) {
	    struct stat st;
	    char path[4096];
	    
	    cfg_dir(peer->cfg, CFG_DIR_GLOBAL, path, sizeof(path));
	    i = stat(path, &st);
	    if( i==0 && st.st_mtime > peer->cfg_dir_time ) {
		/* reload config and log in again if config dir changes */
		p2p_peer_cfg_load(peer);
		peer->cfg_dir_time = st.st_mtime;

		/* start logging in again */
		state_set(&peer->login_state, P2P_PEER_LOGIN_INIT, mstime());

	    }
	    peer->poll_cfg_time_last = peer->mstime;
	}
	
	p2p_router_poll(peer->router);
	p2p_peer_login_poll(peer);
	p2p_peer_dns_proxy_poll(peer);
	p2p_peer_poll_all_servers(peer);
	p2p_peer_client_foreach(peer, p2p_peer_client_foreach_connect_poll, 0);
	
    } while(0);
    return 0;
}

enum {
    P2P_PEER_ARGV_OPT_HOSTNAME=1
    ,P2P_PEER_ARGV_OPT_PASSWORD
    ,P2P_PEER_ARGV_OPT_SAVE_PASSWORD
    ,P2P_PEER_ARGV_OPT_EMAIL
    ,P2P_PEER_ARGV_OPT_DEBUG
    ,P2P_PEER_ARGV_OPT_VERSION
    ,P2P_PEER_ARGV_OPT_HELP
    ,P2P_PEER_ARGV_OPT_LOG_PREFIX
    ,P2P_PEER_ARGV_OPT_CONNECT
    ,P2P_PEER_ARGV_OPT_CFG_NAME
    ,P2P_PEER_ARGV_OPT_CFG_ROOTDIR
    ,P2P_PEER_ARGV_OPT_INSTALL
    ,P2P_PEER_ARGV_OPT_UNINSTALL
    ,P2P_PEER_ARGV_OPT_CTL_PORT
    ,P2P_PEER_ARGV_OPT_NO_ETHERTAP
    ,P2P_PEER_ARGV_OPT_UDP_PORT
    ,P2P_PEER_ARGV_OPT_TCP_PORT
};

opt_set_t
p2p_peer_opts[] = {
    { P2P_PEER_ARGV_OPT_NO_ETHERTAP,     "no-ethertap", OPT_TYPE_FLAG },
    { P2P_PEER_ARGV_OPT_CTL_PORT,        "ctl-port", OPT_TYPE_INT },
    { P2P_PEER_ARGV_OPT_INSTALL,         "install", OPT_TYPE_FLAG },
    { P2P_PEER_ARGV_OPT_UNINSTALL,       "uninstall", OPT_TYPE_FLAG },
    { P2P_PEER_ARGV_OPT_HOSTNAME,        "host", OPT_TYPE_STR },
    { P2P_PEER_ARGV_OPT_PASSWORD,        "pass", OPT_TYPE_STR },
    { P2P_PEER_ARGV_OPT_SAVE_PASSWORD,   "save-pass", OPT_TYPE_FLAG },
    { P2P_PEER_ARGV_OPT_VERSION,         "v", OPT_TYPE_INT },
    { P2P_PEER_ARGV_OPT_HELP,            "h", OPT_TYPE_INT },
    { P2P_PEER_ARGV_OPT_HELP,            "help", OPT_TYPE_INT },
    { P2P_PEER_ARGV_OPT_DEBUG,           "debug", OPT_TYPE_INT },
    { P2P_PEER_ARGV_OPT_DEBUG,           "d", OPT_TYPE_INT },
    { P2P_PEER_ARGV_OPT_LOG_PREFIX,      "log-dir", OPT_TYPE_STR },
    { P2P_PEER_ARGV_OPT_LOG_PREFIX,      "l", OPT_TYPE_STR },
    { P2P_PEER_ARGV_OPT_CONNECT,         "c", OPT_TYPE_STR },
    { P2P_PEER_ARGV_OPT_CFG_NAME,        "cfg-name", OPT_TYPE_STR },
    { P2P_PEER_ARGV_OPT_CFG_ROOTDIR,     "cfg-dir", OPT_TYPE_STR },
    { P2P_PEER_ARGV_OPT_UDP_PORT,        "udp_port", OPT_TYPE_INT },
    { P2P_PEER_ARGV_OPT_TCP_PORT,        "tcp_port", OPT_TYPE_INT },
    OPT_SET_END
};

int
p2p_peer_install(p2p_peer_t *peer) {
    int i, err=-1;
    do {
	/* the setup program has to stop the service before copying this exe */
	p2p_ctl_peer_service_config(P2P_PEER_OPT_RECONNECT_BOOT);

	peer->ethertap = ethertap_new();
	assertb(peer->ethertap);

	i = ethertap_open(peer->ethertap);
	assertb(!i);

	err =0;
    } while(0);
    return err;
}

int
p2p_peer_uninstall(p2p_peer_t *peer) {
    int i, err=-1;
    do {
	printf("Uninstalling p2p_peer service...\n");

	i = p2p_ctl_peer_service_uninstall();

	// todo - remove the ethertap device
	//i = p2p_peer_ethertap_open(peer);
	//assertb(!i);

	printf("done\n");
	err =0;
    } while(0);
    return err;
}


int
proc_daemon() {
#if OS==OS_UNIX 
    pid_t pid;
    int i;
    int fd;
    
    do {
	pid = fork();
	if( pid != 0 ) {
	    break;
	}
	
	// careful to redirect to /dev/null instead of closing 0, 1,
	// 2.  Otherwise new files will be opened to 0, 1, or 2 and
	// get clobbered by stderr or stdout
	fd = open("/dev/null", O_RDWR);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);

	// from http://www.enderunix.org/docs/eng/daemon.php
	for(i=getdtablesize()-1; i>2; i--) {
	    close(i);
	}
	setsid();
	//umask(027);

    } while(0);
    return pid;
#else
    return 0;
#endif
}

int
p2p_peer_cmd(p2p_peer_t *peer, int argc, char **argv,
	     p2p_ctl_peer_conn_t *conn,
	     p2p_ctl_peer_status_t *peer_status
	     ) {
    char *arg = argv[0];
    int i, err=-1;
    char buf1[1024];
    p2p_ctl_peer_t peer_status_msg;
    p2p_ctl_peer_t msg;
    mstime_t t;
    char buf[4096];
    sock_t sock;
    peer_status_msg.type = 0;
    do {
	if( argc <= 0 ) {
	    if( conn ) { 
		arg = "status"; 
	    }
	    else {
		err = 0;
		break;
	    }
	}

	if( strcmp(arg, "status")==0 ) {
	    if( !peer_status ) {
		printf("p2p_peer is not running\n");
	    }
	    else {
		printf("p2p_peer is running\n");
		printf("hostname: %s\n", peer_status->hostname);
		printf("version: %d.%02d\n", 
		       peer_status->ver.major, peer_status->ver.minor);
		printf("expires: %s\n", 
		       mstime_fmt(peer_status->expire, buf1, sizeof(buf1)));
	    }
	    err = 1;
	    break;
	}
	else if( strcmp(arg, "restart")==0 ) {
	    char *argv_start_stop[] = { "stop", "start" };
	    p2p_peer_cmd(peer, 1, &argv_start_stop[0], conn, peer_status);

	    /* broken: for spome reason theis doesn't restart */
	    p2p_peer_cmd(peer, 1, &argv_start_stop[1], 0, 0);
	    err = 1;
	    break;
	}
	else if( strcmp(arg, "start")==0 
		 || strcmp(arg, "daemon")==0 ) {
	    if( conn ) {
		printf("p2p_peer is already running");
		err = 1;
		break;
	    }

	    /* become a daemon */
	    err = proc_daemon();
	    if( err > 0 ) {
		//printf("p2p_peer started pid=%d\n", err);
		t = mstime();
		while(mstime()-t < 20) {
		    conn = p2p_ctl_peer_conn_connect4(0, 0, 0, &peer_status_msg);
		    if( !conn ) {
			mssleep(1);
			continue;
		    }
		    peer_status = &peer_status_msg.p2p_ctl_peer_t_u.peer_status;
		    printf("p2p_peer started hostname=%s\n", peer_status->hostname);
		    t = 0;
		    break;
		}
		if( t ) {
		    printf("failed to start p2p_peer\n");
		}
		err = 1;
	    }
	    /* return err = 0 to continue as child */
	    break;
	}
	else if( strcmp(arg, "stop")==0 ) {
	    if( conn ) {
		/* send shutdown request */
		p2p_ctl_peer_init(&msg, P2P_CTL_PEER_STATUS);  {
		    p2p_ctl_peer_status_t *m = &msg.p2p_ctl_peer_t_u.peer_status;
		    m->ui_state = P2P_UI_CONNECT_CLIENT_CLOSED;
		}
		i = p2p_ctl_peer_conn_send(conn, &msg);
		
		/* wait for it to shut up */
		t=mstime();
		sock = p2p_ctl_peer_conn_sock(conn);
		while(mstime()-t < 20 ) {
		    fd_set rfds;
		    struct timeval tv = {20, 0};
		    
		    FD_ZERO(&rfds);
		    FD_SET(sock, &rfds);
		    i = select(sock+1, &rfds, 0, 0, &tv);
		    if( i<=0 ) {
			break;
		    }
		    i = recv(sock, buf, sizeof(buf), 0);
		    if( i<=0 ) {
			break;
		    }
		}
		close(sock);
		mssleep(2);
	    }
	    printf("p2p_peer stopped\n");
	    err = 1;
	    break;
	}
	if( strcmp(arg, "install")==0 ) {
	    p2p_peer_install(peer);
	    err = 1;
	    break;
	}
	if( strcmp(arg, "uninstall")==0 ) {
	    p2p_peer_uninstall(peer);
	    err = 1;
	    break;
	}
	err = 0;
    } while(0);
    p2p_ctl_peer_free(&peer_status_msg);
    return err;
}


int
p2p_peer_argv(p2p_peer_t *peer, int *argc, char **argv) {
    opt_iter_t opt;
    int i, x, err=-1;
    char *opt_cfg_rootdir = 0;
    char buf1[1024], buf2[1024];
    ifcfg_t *ic=0;
    p2p_ctl_peer_conn_t *conn = 0;
    p2p_ctl_peer_t peer_status_msg;
    p2p_ctl_peer_status_t *peer_status = 0;
    
    peer_status_msg.type = 0;

    opt.argi = 1;
    while(1) {
	err = opt_get_argv(p2p_peer_opts, *argc, argv, &opt);
	if( err <= 0 ) {
	    opt_perror(&opt, stdout);
	    break;
	}
	switch(err) {
	case P2P_PEER_ARGV_OPT_INSTALL:
	    p2p_peer_install(peer);
	    return -2;
	    break;

	case P2P_PEER_ARGV_OPT_UNINSTALL:
	    p2p_peer_uninstall(peer);
	    return -2;
	    break;

	case P2P_PEER_ARGV_OPT_NO_ETHERTAP: 
	    peer->no_ethertap = opt.val.i; 
	    break;

	case P2P_PEER_ARGV_OPT_UDP_PORT: 
	    peer->udp_port = opt.val.i; 
	    break;

	case P2P_PEER_ARGV_OPT_TCP_PORT: 
	    peer->tcp_port = opt.val.i; 
	    break;

	case P2P_PEER_ARGV_OPT_CTL_PORT: 
	    peer->ctl_port = opt.val.i; 
	    break;

	case P2P_PEER_ARGV_OPT_CFG_ROOTDIR: 
	    opt_cfg_rootdir = opt.val.s; 
	    break;

	case P2P_PEER_ARGV_OPT_HOSTNAME:
	    pool_copy_str(peer->pool, &peer->peer_hostname, opt.val.s);
	    break;

	case P2P_PEER_ARGV_OPT_PASSWORD:
	    pool_copy_str(peer->pool, &peer->peer_password, opt.val.s);
	    break;

	case P2P_PEER_ARGV_OPT_DEBUG: 
	    peer->debug_level = opt.val.i;
	    break;

	case P2P_PEER_ARGV_OPT_LOG_PREFIX: 
	    pool_copy_str(peer->pool, &peer->log_prefix, opt.val.s);
	    break;

	case P2P_PEER_ARGV_OPT_SAVE_PASSWORD: {
	    if( opt.val.i ) {
		peer->opts      |= P2P_PEER_OPT_SAVE_PASSWORD;
		peer->opts_argv |= P2P_PEER_OPT_SAVE_PASSWORD;
	    }
	    else {
		peer->opts      &= ~P2P_PEER_OPT_SAVE_PASSWORD;
		peer->opts_argv &= ~P2P_PEER_OPT_SAVE_PASSWORD;
	    }
	    peer->opts_argv_mask |= P2P_PEER_OPT_SAVE_PASSWORD;
	    break;
	}

	case P2P_PEER_ARGV_OPT_CONNECT: {
	    p2p_client_t *client;
	    /* start connecting to a peer */
	    client = p2p_peer_client_get_client_hostname(peer, opt.val.s, 1);
	    assertb(client);
	    state_set(&client->client_connect_state, P2P_PEER_CONNECT_INIT, mstime());
	    break;
	}
	}
    }
    if( err == OPT_ERROR_END ) err = 0;

    do {
	/* connect to existing p2p_peer */
	conn = p2p_ctl_peer_conn_connect4(0, 0, 0, &peer_status_msg);
	if( conn ) {
	    peer_status = &peer_status_msg.p2p_ctl_peer_t_u.peer_status;
	}

	/* handle commands */
	i = p2p_peer_cmd(peer, *argc-opt.argi, argv+opt.argi,
			 conn, peer_status);
if( i != 0 || conn ) {
	    err = -2;
	    break;
	}
	
	err = -1;
	if( opt_cfg_rootdir ) {
	    env_put("P2P_PEER_CFG_DIR", opt_cfg_rootdir);
	}
	peer->cfg = p2p_ctl_peer_cfg();
	assertb(peer->cfg);

	// get debug level
	i = cfg_get_int(peer->cfg, "debug", &x);
	if( i >= 0 ) {
	    peer->debug_level = x;
	}

	// open log
	cfg_dir(peer->cfg, CFG_DIR_LOG, buf2, sizeof(buf2));
	snprintf(buf1, sizeof(buf1), "%s/p2p_peer", buf2);
	pool_copy_str(peer->pool, &peer->log_prefix, buf1);
	i = dir_mkpath(peer->log_prefix, 0);
	assertb(i==0);
	peer->debug_func = debug_func_log;
	peer->debug_arg  = 
	    debug_func_log_init(peer->log_prefix, 5, (int)1e6, stderr);
	debug_init(peer->debug_level, p2p_peer_debug, peer);

	// accept control connections
	if( !peer->ctl_port ) {
	    i = cfg_get_int(peer->cfg, "ctl_port", &peer->ctl_port);
	}
	if( !peer->ctl_port ) {
	    peer->ctl_port = P2P_PEER_CTL_PORT;
	}
	i = p2p_peer_ctl_init(peer);
	assertb(!i);

	i = p2p_peer_cfg_load(peer);
	assertb(i>=0);

	i = p2p_peer_db_open(peer);
	assertb(i>=0);
	
	err = 0;
    } while(0);

    if( peer_status_msg.type ) {
	p2p_ctl_peer_free(&peer_status_msg);
    }
    if( conn ) {
	p2p_ctl_peer_conn_delete(conn);
    }
    return err;
}

#if OS & OS_UNIX 
void
p2p_peer_select_stdin(fdselect_t *sel, fd_t sock, int events, void *arg) {
    p2p_peer_t *peer = (p2p_peer_t*)arg;
    char buf[4096];
    int n;

    *buf=0;
    n = read(sock, buf, sizeof(buf)-1);
    buf[n>=0 ? n : 0] = 0;

    printf("select stdin, n=%d buf=%s\n", n, buf);

    if( n <= 0 ) {
	fdselect_unset(sel, sock, events);
    }
    else if( *buf == 'q' ) {
	peer->exit = 1;
    }
}
#endif

int
p2p_peer_poll_pre_select(p2p_peer_t *peer) {
    if( peer->in_select ) {
	return 0;
    }
    fdselect_select(peer->fdselect, 0);
    return 0;
}

// run forever
int
p2p_peer_run(p2p_peer_t *peer) {
    int i, err=-1;
    mstime_t t;
    p2p_router_t *router = peer->router;
    
    do {
	sig_init();
	
	i = p2p_peer_netcfg_init(peer);
	assertb(i==0);

	// poll forever
	peer->in_select = 1;
	while(!sig_exited && !service_exited) {
	    
	    // debug - looking for the mysterious 600ms delay
	    t = mstime();

	    p2p_peer_poll(peer);

	    // debug - looking for the mysterious 600ms delay
	    t = mstime() - t;
	    if( t > .3 ) {
		debug(DEBUG_WARN, ("p2p_peer_poll took %0.3f secs\n", t));
	    }

	    fdselect_select(peer->fdselect, P2P_PEER_POLL_INTERVAL);
	}
	peer->in_select = 0;
	debug(DEBUG_INFO, ("p2p_peer_run: sig_exited=%d\n", sig_exited));

	/* gracefully close connections */
	p2p_peer_ui(peer, 0, 0, "Closing connections");
	peer->closing = 1;
	t = mstime();
	while(p2p_router_close(peer->router) > 0) {
	    fdselect_select(peer->fdselect, P2P_PEER_POLL_INTERVAL);
	    if( mstime() - t > 30 ) {
		break;
	    }
	    p2p_peer_poll(peer);
	}

	p2p_peer_ui(peer, P2P_UI_CONNECT_CLIENT_CLOSED, 0, "Shutting down");

    } while(0);
    return 0;
}

int
p2p_peer_shutdown(p2p_peer_t *peer) {
    sig_exited = 1;
    return 0;
}


typedef struct {
    p2p_peer_t                   *peer;
    p2p_peer_client_foreach_func  func;
    void                         *arg;
} p2p_peer_client_foreach_arg_t;

int
p2p_peer_foreach_client(hash_t *hash, hash_node_t *node, void *arg) {
    p2p_peer_client_foreach_arg_t *x = (p2p_peer_client_foreach_arg_t*)arg;
    return x->func(x->peer, (p2p_client_t*)node->node_val, x->arg);
}

int
p2p_peer_client_foreach(p2p_peer_t *peer, p2p_peer_client_foreach_func func, void *arg) {
    p2p_peer_client_foreach_arg_t x;
    memset(&x, 0, sizeof(x));
    x.peer = peer;
    x.func = func;
    x.arg = arg;
    return hash_foreach(&peer->client_hash_pseudo_ipaddr, p2p_peer_foreach_client, &x);
}

