#include "bklib/bkwin32.h"
 //////////////////////////////////////////////////////////////////////////////
 // Name:        p2p_ui_frame.cpp
 // Purpose:     
 // Author:      Noel Burton-Krahn
 // Modified by: 
 // Created:     08/17/04 00:48:26
 // RCS-ID:      
 // Copyright:   Copyright Burton-Krahn Inc
 // Licence:     
 /////////////////////////////////////////////////////////////////////////////

 #ifdef __GNUG__
 #pragma implementation "p2p_ui_frame.h"
 #endif

 // For compilers that support precompilation, includes "wx/wx.h".
 #include "wx/wxprec.h"
 #include "wx/splash.h"
 #include "wx/image.h"

 #ifdef __BORLANDC__
 #pragma hdrstop
 #endif

 ////@begin includes
 ////@end includes

 #include "p2p_ui.h"
 #include "wx/socket.h"
 #include "wx/datetime.h"
 #include "wx/hashmap.h"
 #include "wx/mstream.h"
 #include "wx/clipbrd.h"

 #include "p2p_ui_frame.h"
 #include "p2p_ui_accept_dialog.h"
 #include "p2p_ui_connect_dialog.h"
 #include "p2p_ui_taskbar.h"
 #include "p2p_ui_application_list.h"
 #include "p2p_ui_host_list.h"
 #include "p2p_ui_account_html_window.h"
 #include "p2p_ui_about_dialog.h"

 //#include "p2p_ui_client_panel.h"
 //#include "p2p_ui_connect_password.h"
 //#include "p2p_ui_connect_accept.h"
 //#include "p2p_ui_port_user.h"
 //#include "p2p_ui_alert.h"
 #include "keycombobox.h"

 #include "bkwxutil.h"
 #include "bkpopupmenu.h"
 #include "bklib/configbk.h"
 #include "bklib/debug.h"
 #include "bklib/readable.h"
 #include "bklib/defutil.h"
 #include "bklib/service.h"
 #include "bklib/proc.h"
 #include "bklib/shell.h"
 #include "bklib/dir.h"
 #include "bklib/opt.h"

 ////@begin XPM images
 ////@end XPM images



 class p2puiLogDebug :public wxLog {
 public:
     virtual void DoLogString(const wxChar *msg, time_t timestamp) {
	 debug(DEBUG_WARN, ("%s", msg));
     }
 };



 IMPLEMENT_DYNAMIC_CLASS( p2puiFrame, wxFrame )

 /*!
  * p2puiFrame event table definition
  */

BEGIN_EVENT_TABLE( p2puiFrame, wxFrame )

    EVT_MENU(ID_QUIT,  p2puiFrame::OnQuit)
    EVT_MENU(ID_ABOUT, p2puiFrame::OnAbout)
    EVT_SOCKET(ID_SOCKET, p2puiFrame::OnSocket)
    EVT_TIMER(ID_TIMER, p2puiFrame::OnTimer)

    EVT_LISTBOX( ID_CONNECTED_LIST, p2puiFrame::OnConnectedListClick )
    EVT_LISTBOX_DCLICK( ID_CONNECTED_LIST, p2puiFrame::OnConnectedListDClick )
    EVT_LISTBOX_RIGHTCLICK( ID_CONNECTED_LIST, p2puiFrame::OnConnectedListRightClick )

    EVT_LISTBOX( ID_APP_LISTEN_LIST, p2puiFrame::OnAppListenListClick )
    EVT_LISTBOX( ID_APP_CALL_LIST, p2puiFrame::OnAppCallListClick )
    EVT_LISTBOX_DCLICK( ID_CONNECTED_RUN_LIST, p2puiFrame::OnConnectedListDClick )

////@begin p2puiFrame event table entries
   EVT_CLOSE( p2puiFrame::OnCloseWindow )

   EVT_NOTEBOOK_PAGE_CHANGED( ID_NOTEBOOK, p2puiFrame::OnNotebookPageChanged )
   EVT_UPDATE_UI( ID_NOTEBOOK, p2puiFrame::OnNotebookUpdate )

   EVT_BUTTON( ID_ACCOUNT_CONFIGURE, p2puiFrame::OnAccountConfigureClick )

   EVT_BUTTON( ID_CONNECT_BUTTON, p2puiFrame::OnConnectButtonClick )
   EVT_UPDATE_UI( ID_CONNECT_BUTTON, p2puiFrame::OnConnectButtonUpdate )

   EVT_COMBOBOX( ID_CONNECT_TEXT, p2puiFrame::OnConnectTextSelected )

   EVT_BUTTON( ID_DISCONNECT, p2puiFrame::OnDisconnectClick )
   EVT_UPDATE_UI( ID_DISCONNECT, p2puiFrame::OnDisconnectUpdate )

   EVT_BUTTON( ID_CONNECT_RUN_BUTTON, p2puiFrame::OnConnectRunButtonClick )
   EVT_UPDATE_UI( ID_CONNECT_RUN_BUTTON, p2puiFrame::OnConnectRunButtonUpdate )

   EVT_LIST_ITEM_SELECTED( ID_LOGIN_ACCEPT_LIST, p2puiFrame::OnLoginAcceptListSelected )
   EVT_LIST_DELETE_ITEM( ID_LOGIN_ACCEPT_LIST, p2puiFrame::OnLoginAcceptListDeleteItem )

   EVT_LIST_ITEM_SELECTED( ID_LOGIN_CONNECT_LIST, p2puiFrame::OnLoginConnectListSelected )
   EVT_LIST_DELETE_ITEM( ID_LOGIN_CONNECT_LIST, p2puiFrame::OnLoginConnectListDeleteItem )

   EVT_TEXT( ID_LOGIN_HOSTNAME, p2puiFrame::OnLoginHostnameUpdated )

   EVT_BUTTON( ID_LOGIN_SAVE, p2puiFrame::OnLoginSaveClick )
   EVT_UPDATE_UI( ID_LOGIN_SAVE, p2puiFrame::OnLoginSaveUpdate )

   EVT_BUTTON( ID_LOGIN_CLEAR, p2puiFrame::OnLoginClearClick )

   EVT_BUTTON( ID_LOGIN_DELETE, p2puiFrame::OnLoginDeleteClick )

   EVT_CHECKBOX( ID_LOGIN_ACCEPT_LOCAL, p2puiFrame::OnLoginAcceptLocalClick )

   EVT_CHECKBOX( ID_LOGIN_ACCEPT_ANY, p2puiFrame::OnLoginAcceptAnyClick )

   EVT_BUTTON( ID_LOGIN_OPTIONS_SAVE, p2puiFrame::OnLoginOptionsSaveClick )
   EVT_UPDATE_UI( ID_LOGIN_OPTIONS_SAVE, p2puiFrame::OnLoginOptionsSaveUpdate )

   EVT_UPDATE_UI( ID_APP_LABEL, p2puiFrame::OnAppLabelUpdate )

   EVT_BUTTON( ID_APP_ICON, p2puiFrame::OnAppIconClick )

   EVT_UPDATE_UI( ID_APP_CMDLINE, p2puiFrame::OnAppCmdlineUpdate )

   EVT_BUTTON( ID_APP_BROWSE, p2puiFrame::OnAppBrowseClick )

   EVT_BUTTON( ID_APP_ICON_UPDATE, p2puiFrame::OnAppIconUpdateClick )

   EVT_TEXT( ID_APP_PORT, p2puiFrame::OnAppPortUpdated )

   EVT_CHECKBOX( ID_APP_AUTORUN, p2puiFrame::OnAppAutorunClick )

   EVT_BUTTON( ID_APP_SAVE, p2puiFrame::OnAppSaveClick )

   EVT_BUTTON( ID_APP_RUN, p2puiFrame::OnAppRunClick )

   EVT_BUTTON( ID_APP_NEW, p2puiFrame::OnAppNewClick )

   EVT_BUTTON( ID_APP_DELETE, p2puiFrame::OnAppDeleteClick )

   EVT_MENU( ID_MENU_FILE_UPDATES, p2puiFrame::OnMenuFileUpdatesClick )

   EVT_MENU( ID_MENU_FILE_SHUTDOWN, p2puiFrame::OnMenuFileShutdownClick )

   EVT_MENU( ID_MENU_FILE_CLOSE, p2puiFrame::OnMenuFileCloseClick )

   EVT_MENU( ID_MENU_FILE_HIDE, p2puiFrame::OnMenuFileHideClick )

   EVT_MENU( ID_MENU_CONFIGURE_ACCOUNT, p2puiFrame::OnMenuConfigureAccountClick )

   EVT_MENU( ID_MENU_HELP_CONTENTS, p2puiFrame::OnMenuHelpContentsClick )

   EVT_MENU( ID_MENU_HELP_ABOUT, p2puiFrame::OnMenuHelpAboutClick )

////@end p2puiFrame event table entries

END_EVENT_TABLE()

/*!
 * p2puiFrame constructors
 */

p2puiFrame::~p2puiFrame() {

#if wxHAS_TASKBARICON
    if( m_taskbaricon ) {
	delete m_taskbaricon;
	m_taskbaricon = 0;
    }
#endif // wxHAS_TASKBARICON

    m_timer->Stop();
    m_ctl_sock->Close();

    m_conn.RemoveListener(OnCtlPeerRecv_s, this);

    // make sure to delete my children before I delete m_conn
    DestroyChildren();

    if( m_cfg ) {
	cfg_delete(m_cfg);
	m_cfg = 0;
    }
}


wxString
CfgPath(const char *name) {
    cfg_t *cfg=0;
    wxString s;
    char buf[4096];

    do {
	cfg = p2p_ctl_peer_cfg();
	assertb(cfg);
	cfg_path(cfg, (char*)name, buf, sizeof(buf));
	s = wxT(buf);
    } while(0);
    if( cfg ) {
	cfg_delete(cfg);
    }
    return s;
}

#include "icons/p2p_ui.xpm"

cfg_t *
p2puiFrame::GetCfg() {
    if( !m_cfg ) {	
	m_cfg = p2p_ctl_peer_cfg();
    }
    return m_cfg;
}


int
p2puiFrame::TryConnectPeer() {
    int i, err=-1;
    p2p_sockaddr_t addr;

    do {
	// try to connect to p2p_peer or start it
	i = p2p_ctl_peer_conn_addr(0, &addr);
	if( i < 0 ) {
	    wxSleep(1);
	    continue;
	}

	wxIPV4address wxaddr;
	wxaddr.Hostname(iaddr_ntoa(htonl(addr.p2p_sockaddr_u.inet.addr), 0));
	wxaddr.Service(addr.p2p_sockaddr_u.inet.port);

	m_ctl_sock->Notify(TRUE);
	i = m_ctl_sock->Connect(wxaddr, FALSE);

	SetPeerStatus();

	err = 0;
    } while(0);
    return err;
}


int
p2puiFrame::ConnectPeer() {
    wxString s;
    int err=-1;

    m_connect_timeout = mstime() + 20;
    do {
	err = TryConnectPeer();
	if( err == 0 ) break;
    } while(!m_ctl_sock->IsConnected() && mstime() < m_connect_timeout);
    return err;
}


void p2puiFrame::OnQuit(wxCommandEvent& WXUNUSED(event)) {
    // TRUE is to force the frame to close
    Close(TRUE);
}

void
p2puiFrame::OnAbout(wxCommandEvent& WXUNUSED(event)) {
    ShowHelp("cfg:ui/help/about.html");
}

void
p2puiFrame::ShowHelp(const char *page) {
    if( !page ) page = "cfg:ui/help/index.html";
    if( !m_about_dialog ) {
	m_about_dialog = new p2puiAboutDialog(this);
    }
    m_about_dialog->LoadPage(page);
    m_about_dialog->Raise();
    m_about_dialog->Show();
}

void
p2puiFrame::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    do {
	// refresh the list in the current page
	RefreshPage();
    } while(0);
}

void
p2puiFrame::OnConnected() {
    int i, err=-1;
    p2p_ctl_peer_t msg;

    do {
	i = m_conn.Attach(*m_ctl_sock);
	assertb(i>=0);

	m_connect_timeout = -1;
	m_conn.AddListener(OnCtlPeerRecv_s, this);
	p2p_ctl_peer_init(&msg, P2P_CTL_PEER_QUERY);
	{
	    p2p_ctl_peer_query_t *m = &msg.p2p_ctl_peer_t_u.query;
	    m->ctl_username = "";
	    m->ctl_password = "";
	    m->fields |= P2P_CTL_PEER_STATUS_FIELD_DEBUG_MSG
		| P2P_CTL_PEER_STATUS_FIELD_SHOW_UI
		;
	}
	i = m_conn.Send(&msg);
	assertb(i>=0);
	m_client_list_wait = wxDateTime::Now();

	// I don't use the timer, but wxSocket->Write hangs without this. Why?
	m_timer->Start(1000);

	err = 0;
    } while(0);
}

void
p2puiFrame::OnSocket(wxSocketEvent& event) {
    wxString s;
    int i;

    do {
	i = event.GetSocketEvent();

	switch(i) {

	case wxSOCKET_CONNECTION: {
	    OnConnected();
	    break;
	}

	case wxSOCKET_LOST: {
	    wxIPV4address addr;
	    event.GetSocket()->GetPeer(addr);

	    if( m_shutdown ) {
		break;
	    }

	    s = "The " P2PVPN_PRODUCT " network service failed to start."
		"  Would you like to try start it again?"
		"  Click Cancel to close this application.";
	    if( m_connect_timeout < 0 ) {
		s = "The " P2PVPN_PRODUCT " network service stopped unexpectedly."
		    "  Would you like to try start it again?"
		    "  Click Cancel to close this application.";
	    }
	    else if( mstime() < m_connect_timeout )  {
		SetPeerStatus();
		//m_registration_text->SetValue("Starting the " P2PVPN_PRODUCT " network service...");
		i = p2p_ctl_peer_service_start();
		if( i >=0 ) {
		    TryConnectPeer();
		    s = "";
		}
		else {
		}
	    }

	    if( s.Len() ) {
		i = wxMessageBox(s, "Error Connecting to " P2PVPN_PRODUCT " Network Service", 
				 wxOK | wxCANCEL | wxICON_ERROR);
		if( i != wxOK ) {
		    Destroy();
		}
		else {
		    ConnectPeer();
		}
	    }

	    break;
	}

	case wxSOCKET_INPUT: {
	    m_conn.OnReadable();
	    break;
	}

	default:
	    break;
	}
    } while(0);    
}

void
p2puiFrame::StatusMessage(const char *msg) {
    debug(debug_level, ("%s", msg));
}

void
p2puiFrame::OnCtlPeerRecv_s(p2puiCtlPeerConn *conn, p2p_ctl_peer_t *msg, 
			    int incoming, void *arg) {
    ((p2puiFrame*)arg)->OnCtlPeerRecv(conn, msg, incoming);
}

void
p2puiFrame::OnCtlPeerRecv(p2puiCtlPeerConn *WXUNUSED(conn), p2p_ctl_peer_t *msg, int incoming) {
    int i;
    wxString s;

    if( !incoming ) return;

    switch(msg->type) {
    case P2P_CTL_PEER_DEBUG: {	
	p2p_ctl_peer_debug_t *m = &msg->p2p_ctl_peer_t_u.debug;
	i = strlen(m->text)-1;
	if( m->text[i] == '\n' ) { m->text[i] = 0; }
	wxLogMessage("%s", m->text);
	break;
    }

    case P2P_CTL_PEER_EXIT: {
	p2p_ctl_peer_exit_t *m = &msg->p2p_ctl_peer_t_u.exit;
	if( m->code == P2P_CTL_PEER_EXIT_BROKEN ) {
	    s = "The " P2PVPN_PRODUCT " network service stopped unexpectedly."
		"  Would you like to try start it again?"
		"  Click Cancel to close this application.";
	    i = wxMessageBox(s, "Starting " P2PVPN_PRODUCT " Network Service", 
			     wxOK | wxCANCEL | wxICON_ERROR);
	    if( i == wxOK ) {
		Destroy();
	    }
	    else {
		ConnectPeer();
	    }
	}
	else if( m->code == P2P_CTL_PEER_EXIT_NORMAL ) {
	    Destroy();
	}
	break;
    }

    case P2P_CTL_PEER_STATUS: {
	p2p_ctl_peer_status_t *m = &msg->p2p_ctl_peer_t_u.peer_status;
	if( m->ui_state == P2P_UI_CONNECT_CLIENT_CLOSED ) {
	    m_ctl_sock->Close();	    
	    Destroy();
	    return;
	}
	if( m->fields & P2P_CTL_PEER_STATUS_FIELD_SHOW_UI ) {
	    if( m->ui_code ) {
		Show();
		Raise();
	    }
	}

	SetPeerStatus(m);
	break;
    }

    case P2P_CTL_PEER_CLIENT_STATUS: {
	p2p_ctl_peer_client_status_t *client_status = 
	    &msg->p2p_ctl_peer_t_u.client_status;
	p2puiClientDialog *dlg = 0;
	wxString lhostname = client_status->hostname;
	lhostname.MakeLower();
	p2puiClientDialog *old_dlg = m_client_dialog[lhostname];
	int matches = 0;
	p2puiCtlPeerMsg *uimsg = m_conn.GetClientStatus(lhostname);
	p2puiCtlPeerMsg::State state = 
	    uimsg ? uimsg->m_state : p2puiCtlPeerMsg::CLIENT_NONE;


	if( client_status->ui_state == P2P_UI_CONNECT_CLIENT_CLOSED ) {
	    if( m_connected_list->FindHost(lhostname) < 0 ) {
		/* ignore */
		break;
	    }
	}
	else {
	    AddConnectedClient(lhostname);
	}

	if( (
	    strcasecmp(client_status->hostname, 
		       "GSX-4063D7A056.gatekeeper.vpn.natnix.com")==0 
	    ||
	    strcasecmp(client_status->hostname, 
		       "neilsreplicator.gatekeeper.p2p.bkbox.com")==0 
	    )
	    && 
	    !( client_status->ui_code == P2P_ERR_PKT_BLOCKED
		&& client_status->ui_state == P2P_UI_CONNECT_CLIENT_ACCEPTED
		)
	    ) {
	    int debug_break=1;
	}

	switch(client_status->ui_state) {
	case P2P_UI_CONNECT_SERVER: 
	    s.Printf("Connecting to server...");
	    break;

	case P2P_UI_CONNECT_CLIENT: 
	    s.Printf("Calling remote host...");
	    break;

	case P2P_UI_CONNECT_CLIENT_ACCEPT: 
	    s.Printf("Remote Host Accepted...");
	    break;

	case P2P_UI_CONNECT_CLIENT_FAILED: 
	    s.Printf("Failed to connect to remote host."
		     "  Try a different password, and make sure your password is valid on the remote host."
		     );
	    state = p2puiCtlPeerMsg::CLIENT_REJECTED_PASSWORD;
	    dlg = new p2puiConnectDialog(this, s, client_status, 1);
	    break;

	case P2P_UI_CONNECT_CLIENT_CLOSED: 
	    state = p2puiCtlPeerMsg::CLIENT_DISCONNECTED;
	    s.Printf("Connection closed");
	    break;

	case P2P_UI_CLIENT_ACCEPT_CTL:
	    /* Local user must accept incoming username/password */
	    state = p2puiCtlPeerMsg::CLIENT_REQUEST_ACCEPT;
	    s.Printf("Would you like to allow \"%s\" to access this computer?", client_status->client_username);
	    dlg = new p2puiAcceptDialog(this, s, client_status);
	    break;

	case P2P_UI_CONNECT_CLIENT_ACCEPTED:
	    switch( client_status->ui_code ) {
	    case P2P_ERR_PKT_BLOCKED:
		/* remote side blocked connection, you must log in */
		state = p2puiCtlPeerMsg::CLIENT_REJECTED_PASSWORD;
		s = "";
		s << "You must enter a password to connect to the remote computer, either:"
		  << "<ul>"
		  << "<li>a password for an existing account on the remote computer,"
		  << "<li>the remote computer's " P2PVPN_PRODUCT " registration password, or"
		  << "<li>choose a new password and I'll ask the remote user to accept it."
		  << "</ul>"
		  << "The remote user can add a password for you with their " P2PVPN_PRODUCT " control panel."
		  << "<p>"
		  << "The network application that is trying to connect may time out while waiting to authenticate your password.  Just restart the application after your password is accepted."
		    ;
		dlg = new p2puiConnectDialog(this, s, client_status, 1);
		break;

	    default:
		state = p2puiCtlPeerMsg::CLIENT_CONNECTED;
		break;
	    }
	    break;

	case P2P_UI_CONNECT_SERVER_FAILED:
	    switch( client_status->ui_code ) {
	    case P2P_ERR_HOST_NOT_FOUND:
		/* server says this does not exist */
		state = p2puiCtlPeerMsg::CLIENT_DISCONNECTED;
		s.Printf("The remote host is not connected right now. Please try again later.");
		dlg = new p2puiConnectDialog(this, s, client_status, 0);
		break;

	    default:
		/* failed to connect to server itself */
		state = p2puiCtlPeerMsg::CLIENT_DISCONNECTED;
		s.Printf("Failed to connect to remote host's server at \"%s\".  Will try again later", client_status->server_hostname);
		dlg = new p2puiConnectDialog(this, s, client_status, 0);
		break;
	    }
	    break;

	case P2P_UI_CONNECT_SERVER_REJECTED:
	    switch( client_status->ui_code ) {
	    case P2P_ERR_HOST_NOT_FOUND:
		state = p2puiCtlPeerMsg::CLIENT_REJECTED_NOEXIST;
		s.Printf("The remote host does not exist.  Please try a different host name.");
		dlg = new p2puiConnectDialog(this, s, client_status, 0);
		break;

	    default:
		s.Printf("The remote host is not connected right now.  Please try again later.");
		state = p2puiCtlPeerMsg::CLIENT_DISCONNECTED;
		dlg = new p2puiConnectDialog(this, s, client_status, 0);
		break;
	    }
	    break;

	case P2P_UI_CONNECT_CLIENT_REJECTED: {
	    int show_password = 1;
	    state = p2puiCtlPeerMsg::CLIENT_REJECTED_PASSWORD;
	    s = "The remote host rejected your connection";
	    switch( client_status->ui_code ) {
	    case P2P_UI_CLIENT_ACCEPT_CTL:
		s = "The remote user must manually accept your password...";
		break;

	    case P2P_UI_CONNECT_CLIENT_REJECTED:
		s = "The remote user rejected your connection";
		if( *client_status->ui_msg ) {
		    s << ": " << client_status->ui_msg;
		}
		break;

	    case P2P_ERR_LOGIN_EXPIRED:
		s = "Your account at the remote client has expired";
		break;

	    case P2P_ERR_LOGIN_WRONG_PASSWORD:
		if( !*client_status->peer_password ) {
		    s = "You must enter a username and password to connect.";
		}
		else {
		    s = "That password does not match your username.";
		}
		break;
	    default: 
		break;
	    }
	    dlg = new p2puiConnectDialog(this, s, client_status, show_password);
	    break;
	}

	default:
	    s = uimsg->m_state_text;
	    dlg = old_dlg;
	    break;
	}

	uimsg->m_state = state;
	uimsg->m_state_text = s;

	if( dlg != old_dlg ) {
	    matches = old_dlg && old_dlg->Matches(dlg);
	    if( matches ) {
		if( dlg ) {
		    dlg->Destroy();
		}
	    }
	    else {
		/* out with the old dialog show the new dialog */
		if( old_dlg ) {
		    old_dlg->Destroy();
		}
		m_client_dialog[lhostname] = dlg;
		if( dlg ) {
		    dlg->Create(this);
		    dlg->Show();
		    dlg->Raise();
		    dlg->Refresh();
		}
	    }
	}
	break;
    }

    case P2P_CTL_PEER_LIST_END: {
	p2p_ctl_peer_list_end_t *m = &msg->p2p_ctl_peer_t_u.list_end;

	switch(m->type) {
	case P2P_CTL_PEER_CLIENT_STATUS: {
	    /* handled by p2puiClientSet */
	    break;
	}

	case P2P_CTL_PEER_USER_INFO: {
	    p2puiCtlPeerMsgHash::iterator it;
	    p2puiCtlPeerMsg *node;
	    wxListCtrl *list;
	    wxString s;

	    for( it = m_conn.m_user_info_connect.begin(); 
		 it != m_conn.m_user_info_connect.end(); ++it ) {
		node = it->second;
		if( !node || !node->m_deleted ) continue;
		s = node->GetUserInfo()->hostname;
		list = m_login_connect_list;
		i = list->FindItem(-1, s);
		if( i >= 0 ) {
		    list->DeleteItem(i);
		}
	    }

	    for( it = m_conn.m_user_info_accept.begin(); 
		 it != m_conn.m_user_info_accept.end(); ++it ) {
		node = it->second;
		if( !node || !node->m_deleted ) continue;
		s = node->GetUserInfo()->username;
		list = m_login_accept_list;
		i = list->FindItem(-1, s);
		if( i >= 0 ) {
		    list->DeleteItem(i);
		}
	    }
	    break;
	}

	}
	break;
    }

    case P2P_CTL_SEARCH_RESULT: {
	p2p_search_result_t *m = &msg->p2p_ctl_peer_t_u.search_result;
	p2puiSearchResultDialog *dlg = GetSearchResultDialog(m->search_string, 0);
	/* handled by my search result dialogs */
	break;
    }

    case P2P_CTL_PEER_APP_START: {
	p2p_ctl_peer_app_start_t *m = &msg->p2p_ctl_peer_t_u.app_start;

	if( !m->app_info.autorun ) {
	    break;
	}

	/* start app if it's not already running */
	i = p2p_ctl_peer_port_is_not_running(m->app_info.port);
	if( i > 0 ) {
	    m_conn.Run(m->app_info.cmdline, m->client_hostname);
	}
	break;
    }

    case P2P_CTL_PEER_USER_INFO: {
	p2p_ctl_peer_user_info_t *m = &msg->p2p_ctl_peer_t_u.user_info;
	char buf[4096];
	wxListCtrl *list=0;
	int row, col;
	char *s;

	s = m->hostname;
	if( s && *s ) {
	    list = m_login_connect_list;
	}
	else {
	    list = m_login_accept_list;
	    s = m->username;
	}

	// insert into the login list
	row = list->FindItem(-1, s);
	if( row < 0 ) {
	    row = list->GetItemCount();
	    if( row < 0 ) row = 0;
	    row = list->InsertItem(row, s);
	}

	col = 1;
	if( list == m_login_connect_list ) {
	    list->SetItem(row, col, m->username);
	    col++;
	}

	// last time, from connect
	if( m->last_time > 0 ) {
	    list->SetItem(row, col++, readable_date(m->last_time, buf, sizeof(buf)));
	}
	else {
	    list->SetItem(row, col++, "never");
	}

	if( list == m_login_accept_list ) {
	    list->SetItem(row, col++, m->last_from);
	}

	break;
    }

    default:
	break;
    }
}

void
p2puiFrame::RefreshPage() {
    wxNotebookPage *page = m_notebook->GetPage(m_notebook->GetSelection());
    p2p_ctl_peer_t msg;
    int list_type = 0;

    if( !page ) {
	return;
    }

    switch(page->GetId()) {
    case ID_LOGIN_PANEL:
	list_type = P2P_CTL_PEER_USER_INFO;
	break;

    case ID_APP_PANEL:
	list_type = P2P_CTL_PEER_APP_INFO;
	break;

    case ID_CONNECTED_PANEL:
	/* send both app_info and client_status */
	list_type = P2P_CTL_PEER_APP_INFO;
	p2p_ctl_peer_init(&msg, P2P_CTL_PEER_LIST_GET);
	msg.p2p_ctl_peer_t_u.list_get.type = list_type;
	m_conn.Send(&msg);

	list_type = P2P_CTL_PEER_CLIENT_STATUS;
	break;

    default:
	list_type = 0;
	break;
    }

    if( list_type ) {
	p2p_ctl_peer_init(&msg, P2P_CTL_PEER_LIST_GET);
	msg.p2p_ctl_peer_t_u.list_get.type = list_type;
	m_conn.Send(&msg);
    }

}

/*!
 * p2puiFrame constructors
 */

p2puiFrame::p2puiFrame( )
{
    m_cfg = 0;
#if wxHAS_TASKBARICON
    m_taskbaricon = 0;
#endif // wxHAS_TASKBARICON
    m_about_dialog = 0;
}

p2puiFrame::p2puiFrame( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
#if wxHAS_TASKBARICON
    m_taskbaricon = 0;
#endif // wxHAS_TASKBARICON

    m_cfg = p2p_ctl_peer_cfg();
    m_connect_timeout = 0;
    m_shutdown = 0;

    Create( parent, id, caption, pos, size, style );
}

/*!
 * p2puiFrame creator
 */

bool p2puiFrame::Create( wxWindow* parent, wxWindowID id, const wxString& caption, const wxPoint& pos, const wxSize& size, long style )
{
    m_account_dialog = 0;
    m_about_dialog = 0;
    int i;

////@begin p2puiFrame member initialisation
   m_notebook = NULL;
   m_account_html = NULL;
   m_connect_button = NULL;
   m_connect_text = NULL;
   m_connected_list = NULL;
   m_disconnect_button = NULL;
   m_connected_run_list = NULL;
   m_connected_run_button = NULL;
   m_connect_html_help = NULL;
   m_login_accept_list = NULL;
   m_login_connect_list = NULL;
   m_accept_password_box = NULL;
   m_login_accept_radio = NULL;
   m_login_connect_radio = NULL;
   m_login_hostname = NULL;
   m_login_username = NULL;
   m_login_password = NULL;
   m_login_confirm = NULL;
   m_login_save = NULL;
   m_login_accept_existing_check = NULL;
   m_login_accept_any_check = NULL;
   m_login_options_save = NULL;
   m_login_html_help = NULL;
   m_app_call_list = NULL;
   m_app_listen_list = NULL;
   m_app_label = NULL;
   m_app_icon = NULL;
   m_app_cmdline = NULL;
   m_app_port = NULL;
   m_app_port_panel = NULL;
   m_app_allow_public = NULL;
   m_app_autorun = NULL;
   m_app_save = NULL;
////@end p2puiFrame member initialisation

    wxSplashScreen *splash=0;
    wxFileSystem fs;
    wxFSFile* f;
    f = fs.OpenFile("cfg:ui/img/splash.png");
    if( f ) {
	wxImage img;
	if( f->GetStream() ) {
	    img.LoadFile(*(f->GetStream()));
	}
	delete f;
	splash = new wxSplashScreen(img,
				    wxSPLASH_CENTRE_ON_SCREEN
				    | wxSPLASH_NO_TIMEOUT,
				    6000, NULL, -1,
				    wxDefaultPosition, wxDefaultSize,
				    wxSIMPLE_BORDER);
    }
    wxYield();


    wxImage img;
    f = fs.OpenFile("cfg:ui/img/p2p_ui.png");
    if( f ) {
	if( f->GetStream() ) {
	    img.LoadFile(*(f->GetStream()));
	}
	delete f;
    }

    wxIcon icon;
#ifdef __WXMSW__
    icon.LoadFile(CfgPath("ui/img/p2p_ui.ico"), wxBITMAP_TYPE_ICO);
#endif
#ifdef __WXGTK__
    icon.LoadFile(CfgPath("ui/img/p2p_ui.png"), wxBITMAP_TYPE_PNG);
#endif
    i = icon.Ok();

#if wxHAS_TASKBARICON
    m_taskbaricon = new p2puiTaskBarIcon(this);
    m_taskbaricon->SetIcon(icon, P2PVPN_PRODUCT " Control Panel");

#endif // wxHAS_TASKBARICON

////@begin p2puiFrame creation
   wxFrame::Create( parent, id, caption, pos, size, style );

   CreateControls();
   GetSizer()->Fit(this);
   GetSizer()->SetSizeHints(this);
////@end p2puiFrame creation

    if( splash ) {
	splash->Destroy();
    }

    //GetSizer()->Layout();
    //GetSizer()->Fit(this);
    //SetSize(-1, -1, 580, 460);
    SetTitle(P2PVPN_PRODUCT " Control Panel");

    i = icon.Ok();
    i = icon.GetWidth();
    SetIcon(icon);

    wxString s;
    m_connect_html_help->SetBorders(2);
    s = "";
    s <<
	"<table border=0 cellspacing=0>"
	"  <tr>"
	"    <td valign=top><img src=\"cfg:ui/img/32/gears.png\"></td>"
	"    <td valign=top>"
	""
	"      Select a host and an application and double-click for quick launch."
	""
	"      <br>Or, you can enter a hostname or IP address above directly"
	"      into any network application to connect."
	""
	"    </td>"
	"  </tr>"
	"</table>"
	""
	;
    m_connect_html_help->SetPage(s);

    m_login_html_help->SetBorders(2);
    s = "";
    s <<
	"<table border=0 cellspacing=0>"
	"  <tr>"
	"    <td valign=top><img src=\"cfg:ui/img/32/lock.png\"></td>"
	"    <td valign=top>"
	"      Remote hosts need a password to connect.  That password"
	" can be for an existing system account (in this computer's password"
	" database) or a password saved in the " P2PVPN_PRODUCT " database here."
	"    </td>"
	"  </tr>"
	"</table>"
	""
	;
    m_login_html_help->SetPage(s);

    return TRUE;
}

/*!
 * Control creation for p2puiFrame
 */

void p2puiFrame::CreateControls()
{    
////@begin p2puiFrame content construction

   p2puiFrame* itemFrame1 = this;

   wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);
   itemFrame1->SetSizer(itemBoxSizer2);

   m_notebook = new wxNotebook( itemFrame1, ID_NOTEBOOK, wxDefaultPosition, wxDefaultSize, wxNB_TOP );

   wxPanel* itemPanel4 = new wxPanel( m_notebook, ID_ACCOUNT_PANEL, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
   wxBoxSizer* itemBoxSizer5 = new wxBoxSizer(wxVERTICAL);
   itemPanel4->SetSizer(itemBoxSizer5);

   wxStaticBox* itemStaticBoxSizer6Static = new wxStaticBox(itemPanel4, wxID_ANY, _("This computer's account"));
   wxStaticBoxSizer* itemStaticBoxSizer6 = new wxStaticBoxSizer(itemStaticBoxSizer6Static, wxVERTICAL);
   itemBoxSizer5->Add(itemStaticBoxSizer6, 1, wxGROW|wxALL, 2);
   m_account_html = new p2puiAccountHtmlWindow( itemPanel4, ID_FRAME_ACCOUNT_HTML, wxDefaultPosition, wxSize(-1, 80), wxSIMPLE_BORDER|wxHSCROLL|wxVSCROLL );
   itemStaticBoxSizer6->Add(m_account_html, 1, wxGROW|wxALL|wxADJUST_MINSIZE, 2);

   wxButton* itemButton8 = new wxButton( itemPanel4, ID_ACCOUNT_CONFIGURE, _("Account Setup..."), wxDefaultPosition, wxDefaultSize, 0 );
   itemStaticBoxSizer6->Add(itemButton8, 0, wxALIGN_RIGHT|wxALL, 5);

   m_notebook->AddPage(itemPanel4, _("Account"));

   wxPanel* itemPanel9 = new wxPanel( m_notebook, ID_CONNECTED_PANEL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
   wxBoxSizer* itemBoxSizer10 = new wxBoxSizer(wxVERTICAL);
   itemPanel9->SetSizer(itemBoxSizer10);

   wxSplitterWindow* itemSplitterWindow11 = new wxSplitterWindow( itemPanel9, ID_SPLITTERWINDOW, wxDefaultPosition, wxDefaultSize, wxSP_3DBORDER|wxSP_3DSASH|wxNO_BORDER );

   wxPanel* itemPanel12 = new wxPanel( itemSplitterWindow11, ID_CONNECTED_CLIENT_PANEL, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
   wxBoxSizer* itemBoxSizer13 = new wxBoxSizer(wxVERTICAL);
   itemPanel12->SetSizer(itemBoxSizer13);

   wxBoxSizer* itemBoxSizer14 = new wxBoxSizer(wxHORIZONTAL);
   itemBoxSizer13->Add(itemBoxSizer14, 0, wxGROW|wxALL, 2);
   m_connect_button = new wxButton( itemPanel12, ID_CONNECT_BUTTON, _("Search / Connect"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer14->Add(m_connect_button, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 2);

   wxString* m_connect_textStrings = NULL;
   m_connect_text = new KeyComboBox( itemPanel12, ID_CONNECT_TEXT, _T(""), wxDefaultPosition, wxDefaultSize, 0, m_connect_textStrings, wxCB_DROPDOWN|wxWANTS_CHARS );
   itemBoxSizer14->Add(m_connect_text, 1, wxALIGN_CENTER_VERTICAL|wxALL, 2);

   wxStaticText* itemStaticText17 = new wxStaticText( itemPanel12, wxID_STATIC, _("Connected computers"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer13->Add(itemStaticText17, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 2);

   m_connected_list = new p2puiHostListBox( itemPanel12, ID_CONNECTED_LIST, wxDefaultPosition, wxSize(100, 100), wxSIMPLE_BORDER|wxLB_MULTIPLE );
   itemBoxSizer13->Add(m_connected_list, 1, wxGROW|wxALL, 5);

   m_disconnect_button = new wxButton( itemPanel12, ID_DISCONNECT, _("Disconnect Selected"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer13->Add(m_disconnect_button, 0, wxALIGN_RIGHT|wxALL, 5);

   wxPanel* itemPanel20 = new wxPanel( itemSplitterWindow11, ID_CONNECTED_RUN_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
   wxBoxSizer* itemBoxSizer21 = new wxBoxSizer(wxVERTICAL);
   itemPanel20->SetSizer(itemBoxSizer21);

   wxBoxSizer* itemBoxSizer22 = new wxBoxSizer(wxHORIZONTAL);
   itemBoxSizer21->Add(itemBoxSizer22, 0, wxGROW|wxALL, 2);
   itemBoxSizer22->Add(5, 20, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

   wxStaticText* itemStaticText24 = new wxStaticText( itemPanel20, wxID_STATIC, _("Run applications (select and double click)"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer21->Add(itemStaticText24, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 2);

   m_connected_run_list = new p2puiApplicationListPanel( itemPanel20, ID_CONNECTED_RUN_LIST, wxDefaultPosition, wxSize(100, 100), wxNO_BORDER );
   itemBoxSizer21->Add(m_connected_run_list, 1, wxGROW|wxALL, 2);

   m_connected_run_button = new wxButton( itemPanel20, ID_CONNECT_RUN_BUTTON, _("Run Application"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer21->Add(m_connected_run_button, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

   itemSplitterWindow11->SplitVertically(itemPanel12, itemPanel20, 480);
   itemBoxSizer10->Add(itemSplitterWindow11, 1, wxGROW|wxALL, 5);

   m_connect_html_help = new wxHtmlWindow( itemPanel9, ID_CONNECT_HTML_HELP, wxDefaultPosition, wxSize(200, 60), wxHW_SCROLLBAR_AUTO|wxSIMPLE_BORDER|wxHSCROLL|wxVSCROLL );
   itemBoxSizer10->Add(m_connect_html_help, 0, wxGROW|wxALL, 5);

   m_notebook->AddPage(itemPanel9, _("Connections"));

   wxPanel* itemPanel28 = new wxPanel( m_notebook, ID_LOGIN_PANEL, wxDefaultPosition, wxSize(100, 80), wxNO_BORDER|wxTAB_TRAVERSAL );
   wxBoxSizer* itemBoxSizer29 = new wxBoxSizer(wxVERTICAL);
   itemPanel28->SetSizer(itemBoxSizer29);

   wxSplitterWindow* itemSplitterWindow30 = new wxSplitterWindow( itemPanel28, ID_SPLITTERWINDOW1, wxDefaultPosition, wxSize(100, 100), wxSP_3DBORDER|wxSP_3DSASH|wxNO_BORDER );

   wxPanel* itemPanel31 = new wxPanel( itemSplitterWindow30, ID_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
   wxBoxSizer* itemBoxSizer32 = new wxBoxSizer(wxVERTICAL);
   itemPanel31->SetSizer(itemBoxSizer32);

   wxStaticText* itemStaticText33 = new wxStaticText( itemPanel31, wxID_STATIC, _("Accept these usernames and passwords from remote computers."), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer32->Add(itemStaticText33, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 2);

   m_login_accept_list = new wxListCtrl( itemPanel31, ID_LOGIN_ACCEPT_LIST, wxDefaultPosition, wxSize(100, 100), wxLC_REPORT );
   itemBoxSizer32->Add(m_login_accept_list, 1, wxGROW|wxALL, 2);

   itemBoxSizer32->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

   wxStaticText* itemStaticText36 = new wxStaticText( itemPanel31, wxID_STATIC, _("Remember to send these passwords when connecting to remote computers"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer32->Add(itemStaticText36, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 2);

   m_login_connect_list = new wxListCtrl( itemPanel31, ID_LOGIN_CONNECT_LIST, wxDefaultPosition, wxSize(100, 100), wxLC_REPORT );
   itemBoxSizer32->Add(m_login_connect_list, 1, wxGROW|wxALL, 2);

   wxPanel* itemPanel38 = new wxPanel( itemSplitterWindow30, ID_PANEL1, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
   wxBoxSizer* itemBoxSizer39 = new wxBoxSizer(wxVERTICAL);
   itemPanel38->SetSizer(itemBoxSizer39);

   m_accept_password_box = new wxStaticBox(itemPanel38, ID_ACCEPT_PASSWORD_BOX, _("Accept incoming passwords or remember outgoing passwords"));
   wxStaticBoxSizer* itemStaticBoxSizer40 = new wxStaticBoxSizer(m_accept_password_box, wxVERTICAL);
   itemBoxSizer39->Add(itemStaticBoxSizer40, 0, wxGROW|wxALL, 5);
   m_login_accept_radio = new wxRadioButton( itemPanel38, ID_LOGIN_ACCEPT_RADIO, _("Accept this password from remote users (leave hostname blank)"), wxDefaultPosition, wxDefaultSize, 0 );
   m_login_accept_radio->SetValue(FALSE);
   itemStaticBoxSizer40->Add(m_login_accept_radio, 0, wxALIGN_LEFT|wxALL, 5);

   m_login_connect_radio = new wxRadioButton( itemPanel38, ID_LOGIN_CONNECT_RADIO, _("Send this password when connecting to a remote host"), wxDefaultPosition, wxDefaultSize, 0 );
   m_login_connect_radio->SetValue(FALSE);
   itemStaticBoxSizer40->Add(m_login_connect_radio, 0, wxALIGN_LEFT|wxALL, 5);

   wxFlexGridSizer* itemFlexGridSizer43 = new wxFlexGridSizer(2, 2, 0, 0);
   itemFlexGridSizer43->AddGrowableCol(1);
   itemStaticBoxSizer40->Add(itemFlexGridSizer43, 0, wxGROW|wxALL|wxADJUST_MINSIZE, 0);
   wxStaticText* itemStaticText44 = new wxStaticText( itemPanel38, wxID_STATIC, _("Hostname"), wxDefaultPosition, wxDefaultSize, 0 );
   itemFlexGridSizer43->Add(itemStaticText44, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

   m_login_hostname = new wxTextCtrl( itemPanel38, ID_LOGIN_HOSTNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
   itemFlexGridSizer43->Add(m_login_hostname, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 2);

   wxStaticText* itemStaticText46 = new wxStaticText( itemPanel38, wxID_STATIC, _("Username"), wxDefaultPosition, wxDefaultSize, 0 );
   itemFlexGridSizer43->Add(itemStaticText46, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

   m_login_username = new wxTextCtrl( itemPanel38, ID_LOGIN_USERNAME, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
   itemFlexGridSizer43->Add(m_login_username, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 2);

   wxStaticText* itemStaticText48 = new wxStaticText( itemPanel38, wxID_STATIC, _("Password"), wxDefaultPosition, wxDefaultSize, 0 );
   itemFlexGridSizer43->Add(itemStaticText48, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

   m_login_password = new wxTextCtrl( itemPanel38, ID_LOGIN_PASSWORD, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
   itemFlexGridSizer43->Add(m_login_password, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 2);

   wxStaticText* itemStaticText50 = new wxStaticText( itemPanel38, wxID_STATIC, _("Confirm"), wxDefaultPosition, wxDefaultSize, 0 );
   itemFlexGridSizer43->Add(itemStaticText50, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

   m_login_confirm = new wxTextCtrl( itemPanel38, ID_LOGIN_CONFIRM, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
   itemFlexGridSizer43->Add(m_login_confirm, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 2);

   wxBoxSizer* itemBoxSizer52 = new wxBoxSizer(wxHORIZONTAL);
   itemStaticBoxSizer40->Add(itemBoxSizer52, 0, wxGROW|wxALL, 5);
   m_login_save = new wxButton( itemPanel38, ID_LOGIN_SAVE, _("Save"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer52->Add(m_login_save, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

   wxButton* itemButton54 = new wxButton( itemPanel38, ID_LOGIN_CLEAR, _("Create New"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer52->Add(itemButton54, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

   itemBoxSizer52->Add(1, 1, 1, wxALIGN_CENTER_VERTICAL|wxALL, 0);

   wxButton* itemButton56 = new wxButton( itemPanel38, ID_LOGIN_DELETE, _("Delete"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer52->Add(itemButton56, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxADJUST_MINSIZE, 2);

   itemBoxSizer39->Add(5, 5, 1, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

   wxStaticBox* itemStaticBoxSizer58Static = new wxStaticBox(itemPanel38, wxID_ANY, _("More password options"));
   wxStaticBoxSizer* itemStaticBoxSizer58 = new wxStaticBoxSizer(itemStaticBoxSizer58Static, wxVERTICAL);
   itemBoxSizer39->Add(itemStaticBoxSizer58, 0, wxGROW|wxRIGHT|wxTOP|wxBOTTOM, 5);
   m_login_accept_existing_check = new wxCheckBox( itemPanel38, ID_LOGIN_ACCEPT_LOCAL, _("Accept passwords for existing accounts on this computer"), wxDefaultPosition, wxDefaultSize, 0 );
   m_login_accept_existing_check->SetValue(FALSE);
   itemStaticBoxSizer58->Add(m_login_accept_existing_check, 0, wxGROW|wxALL, 5);

   m_login_accept_any_check = new wxCheckBox( itemPanel38, ID_LOGIN_ACCEPT_ANY, _("Insecure! Accept all connections, don't require a password. "), wxDefaultPosition, wxDefaultSize, 0 );
   m_login_accept_any_check->SetValue(FALSE);
   itemStaticBoxSizer58->Add(m_login_accept_any_check, 0, wxGROW|wxALL, 5);

   m_login_options_save = new wxButton( itemPanel38, ID_LOGIN_OPTIONS_SAVE, _("Apply"), wxDefaultPosition, wxDefaultSize, 0 );
   itemStaticBoxSizer58->Add(m_login_options_save, 0, wxALIGN_CENTER_HORIZONTAL|wxALL|wxADJUST_MINSIZE, 2);

   itemSplitterWindow30->SplitVertically(itemPanel31, itemPanel38, 350);
   itemBoxSizer29->Add(itemSplitterWindow30, 1, wxGROW|wxALL, 5);

   m_login_html_help = new wxHtmlWindow( itemPanel28, ID_LOGIN_HTML_HELP, wxDefaultPosition, wxSize(200, 60), wxHW_SCROLLBAR_AUTO|wxSIMPLE_BORDER|wxHSCROLL|wxVSCROLL );
   itemBoxSizer29->Add(m_login_html_help, 0, wxGROW|wxALL, 5);

   m_notebook->AddPage(itemPanel28, _("Passwords"));

   wxPanel* itemPanel63 = new wxPanel( m_notebook, ID_APP_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
   wxBoxSizer* itemBoxSizer64 = new wxBoxSizer(wxVERTICAL);
   itemPanel63->SetSizer(itemBoxSizer64);

   wxSplitterWindow* itemSplitterWindow65 = new wxSplitterWindow( itemPanel63, ID_SPLITTERWINDOW2, wxDefaultPosition, wxSize(100, 100), wxSP_3DBORDER|wxSP_3DSASH|wxNO_BORDER );

   wxPanel* itemPanel66 = new wxPanel( itemSplitterWindow65, ID_PANEL2, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
   wxBoxSizer* itemBoxSizer67 = new wxBoxSizer(wxVERTICAL);
   itemPanel66->SetSizer(itemBoxSizer67);

   wxStaticText* itemStaticText68 = new wxStaticText( itemPanel66, wxID_STATIC, _("Run applications to connect to remote hosts"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer67->Add(itemStaticText68, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

   m_app_call_list = new p2puiApplicationListPanel( itemPanel66, ID_APP_CALL_LIST, wxDefaultPosition, wxSize(100, 100), wxNO_BORDER );
   itemBoxSizer67->Add(m_app_call_list, 1, wxGROW|wxALL, 5);

   itemBoxSizer67->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

   wxStaticText* itemStaticText71 = new wxStaticText( itemPanel66, wxID_STATIC, _("Accept connections from remote hosts"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer67->Add(itemStaticText71, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 5);

   m_app_listen_list = new p2puiApplicationListPanel( itemPanel66, ID_APP_LISTEN_LIST, wxDefaultPosition, wxSize(100, 100), wxNO_BORDER );
   itemBoxSizer67->Add(m_app_listen_list, 1, wxGROW|wxALL, 5);

   wxPanel* itemPanel73 = new wxPanel( itemSplitterWindow65, ID_PANEL3, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
   wxBoxSizer* itemBoxSizer74 = new wxBoxSizer(wxVERTICAL);
   itemPanel73->SetSizer(itemBoxSizer74);

   wxStaticBox* itemStaticBoxSizer75Static = new wxStaticBox(itemPanel73, wxID_ANY, _("Accept connections to a local application"));
   wxStaticBoxSizer* itemStaticBoxSizer75 = new wxStaticBoxSizer(itemStaticBoxSizer75Static, wxVERTICAL);
   itemBoxSizer74->Add(itemStaticBoxSizer75, 1, wxGROW|wxALL, 5);
   wxStaticText* itemStaticText76 = new wxStaticText( itemPanel73, wxID_STATIC, _("Label"), wxDefaultPosition, wxDefaultSize, 0 );
   itemStaticBoxSizer75->Add(itemStaticText76, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 2);

   m_app_label = new wxTextCtrl( itemPanel73, ID_APP_LABEL, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
   itemStaticBoxSizer75->Add(m_app_label, 0, wxGROW|wxALL, 2);

   wxBoxSizer* itemBoxSizer78 = new wxBoxSizer(wxHORIZONTAL);
   itemStaticBoxSizer75->Add(itemBoxSizer78, 0, wxGROW|wxALL, 2);
   wxBitmap m_app_iconBitmap(wxNullBitmap);
   m_app_icon = new wxBitmapButton( itemPanel73, ID_APP_ICON, m_app_iconBitmap, wxDefaultPosition, wxSize(48, 48), wxBU_AUTODRAW|wxBU_EXACTFIT );
   itemBoxSizer78->Add(m_app_icon, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

   wxBoxSizer* itemBoxSizer80 = new wxBoxSizer(wxVERTICAL);
   itemBoxSizer78->Add(itemBoxSizer80, 1, wxALL, 5);
   wxStaticText* itemStaticText81 = new wxStaticText( itemPanel73, wxID_STATIC, _("Command Line"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer80->Add(itemStaticText81, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 2);

   wxBoxSizer* itemBoxSizer82 = new wxBoxSizer(wxHORIZONTAL);
   itemBoxSizer80->Add(itemBoxSizer82, 0, wxGROW|wxALL, 5);
   m_app_cmdline = new wxTextCtrl( itemPanel73, ID_APP_CMDLINE, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer82->Add(m_app_cmdline, 1, wxALIGN_CENTER_VERTICAL|wxALL, 2);

   wxButton* itemButton84 = new wxButton( itemPanel73, ID_APP_BROWSE, _("Browse..."), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer82->Add(itemButton84, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

   wxButton* itemButton85 = new wxButton( itemPanel73, ID_APP_ICON_UPDATE, _("Update Icon"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer80->Add(itemButton85, 0, wxALIGN_LEFT|wxALL, 5);

   wxBoxSizer* itemBoxSizer86 = new wxBoxSizer(wxHORIZONTAL);
   itemStaticBoxSizer75->Add(itemBoxSizer86, 0, wxGROW|wxALL, 2);
   wxBoxSizer* itemBoxSizer87 = new wxBoxSizer(wxVERTICAL);
   itemBoxSizer86->Add(itemBoxSizer87, 1, wxALIGN_TOP|wxALL, 2);
   wxStaticText* itemStaticText88 = new wxStaticText( itemPanel73, wxID_STATIC, _("Listens on IP Port"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer87->Add(itemStaticText88, 0, wxALIGN_LEFT|wxALL|wxADJUST_MINSIZE, 2);

   wxString* m_app_portStrings = NULL;
   m_app_port = new wxComboBox( itemPanel73, ID_APP_PORT, _T(""), wxDefaultPosition, wxDefaultSize, 0, m_app_portStrings, wxCB_DROPDOWN );
   itemBoxSizer87->Add(m_app_port, 0, wxGROW|wxALL, 5);

   m_app_port_panel = new wxPanel( itemPanel73, ID_APP_PORT_PANEL, wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxTAB_TRAVERSAL );
   itemBoxSizer86->Add(m_app_port_panel, 0, wxGROW|wxALL, 5);
   wxBoxSizer* itemBoxSizer91 = new wxBoxSizer(wxVERTICAL);
   m_app_port_panel->SetSizer(itemBoxSizer91);

   m_app_allow_public = new wxCheckBox( m_app_port_panel, ID_APP_PUBLIC, _("Does not require password"), wxDefaultPosition, wxDefaultSize, 0 );
   m_app_allow_public->SetValue(FALSE);
   itemBoxSizer91->Add(m_app_allow_public, 0, wxGROW|wxALL, 2);

   m_app_autorun = new wxCheckBox( m_app_port_panel, ID_APP_AUTORUN, _("Start Automatically"), wxDefaultPosition, wxDefaultSize, 0 );
   m_app_autorun->SetValue(FALSE);
   itemBoxSizer91->Add(m_app_autorun, 0, wxGROW|wxALL, 2);

   wxBoxSizer* itemBoxSizer94 = new wxBoxSizer(wxHORIZONTAL);
   itemStaticBoxSizer75->Add(itemBoxSizer94, 0, wxGROW|wxALL, 5);
   m_app_save = new wxButton( itemPanel73, ID_APP_SAVE, _("&Save"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer94->Add(m_app_save, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

   wxButton* itemButton96 = new wxButton( itemPanel73, ID_APP_RUN, _("Start &Now"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer94->Add(itemButton96, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

   itemBoxSizer94->Add(5, 5, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5);

   wxButton* itemButton98 = new wxButton( itemPanel73, ID_APP_NEW, _("&Clear"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer94->Add(itemButton98, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

   wxButton* itemButton99 = new wxButton( itemPanel73, ID_APP_DELETE, _("&Delete"), wxDefaultPosition, wxDefaultSize, 0 );
   itemBoxSizer94->Add(itemButton99, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

   itemSplitterWindow65->SplitVertically(itemPanel66, itemPanel73, 200);
   itemBoxSizer64->Add(itemSplitterWindow65, 1, wxGROW|wxALL, 5);

   wxBoxSizer* itemBoxSizer100 = new wxBoxSizer(wxVERTICAL);
   itemBoxSizer64->Add(itemBoxSizer100, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

   m_notebook->AddPage(itemPanel63, _("Applications"));

   itemBoxSizer2->Add(m_notebook, 1, wxGROW|wxALL|wxADJUST_MINSIZE, 0);

   wxMenuBar* menuBar = new wxMenuBar;
   wxMenu* itemMenu102 = new wxMenu;
   itemMenu102->Append(ID_MENU_FILE_UPDATES, _("&Check for Updates"), _T(""), wxITEM_NORMAL);
   itemMenu102->AppendSeparator();
   itemMenu102->Append(ID_MENU_FILE_SHUTDOWN, _("Shut &Down Network"), _T(""), wxITEM_NORMAL);
   itemMenu102->Append(ID_MENU_FILE_CLOSE, _("&Quit Control Panel"), _T(""), wxITEM_NORMAL);
   itemMenu102->AppendSeparator();
   itemMenu102->Append(ID_MENU_FILE_HIDE, _("&Hide Control Panel"), _T(""), wxITEM_NORMAL);
   menuBar->Append(itemMenu102, _("&File"));
   wxMenu* itemMenu109 = new wxMenu;
   itemMenu109->Append(ID_MENU_CONFIGURE_ACCOUNT, _("&Account"), _T(""), wxITEM_NORMAL);
   menuBar->Append(itemMenu109, _("&Configure"));
   wxMenu* itemMenu111 = new wxMenu;
   itemMenu111->Append(ID_MENU_HELP_CONTENTS, _("&Contents"), _T(""), wxITEM_NORMAL);
   itemMenu111->Append(ID_MENU_HELP_ABOUT, _("&About"), _T(""), wxITEM_NORMAL);
   menuBar->Append(itemMenu111, _("&Help"));
   itemFrame1->SetMenuBar(menuBar);

////@end p2puiFrame content construction

    int i;

    //----------------------------------------
    // m_log_text
    delete wxLog::SetActiveTarget(new p2puiLogDebug());
    //m_log_text->SetInsertionPointEnd();

    m_connect_text->SetEvent(new wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED, ID_CONNECT_BUTTON));
    MruCombo(m_connect_text, m_cfg, "connected_mru", 0);

    //----------------------------------------
    // m_login_accept_list
    i=0;
    m_login_accept_list->InsertColumn(i++, "User Name");
    m_login_accept_list->InsertColumn(i++, "Last Used");
    m_login_accept_list->InsertColumn(i++, "Last From", wxLIST_FORMAT_LEFT, 180);

    //----------------------------------------
    // m_login_connect_list
    i=0;
    m_login_connect_list->InsertColumn(i++, "Host Name", wxLIST_FORMAT_LEFT, 180);
    m_login_connect_list->InsertColumn(i++, "User Name");
    m_login_connect_list->InsertColumn(i++, "Last Used");

    //----------------------------------------
    // m_ctl_sock 
    m_ctl_sock = new wxSocketClient();
    m_ctl_sock->SetEventHandler(*this, ID_SOCKET);
    m_ctl_sock->SetNotify(wxSOCKET_CONNECTION_FLAG |
			  wxSOCKET_INPUT_FLAG |
			  wxSOCKET_LOST_FLAG);
    m_ctl_sock->Notify(TRUE);
    m_ctl_sock->SetFlags(wxSOCKET_NOWAIT);

    m_timer = new wxTimer(this, ID_TIMER);

    //----------------------------------------
    // m_update 
    m_update = new p2puiUpdateDialog(this);

    //---------------------------------------------------------------------
    // m_connected_list
    m_connected_list->SetConn(GetConn());
    char buf[4096], hostname[4096], *p;
    cfg_get(m_cfg, "ui/connected_list", buf, sizeof(buf));
    p = buf;
    while( cfg_list_next(&p, hostname, sizeof(hostname)) ) {
	AddConnectedClient(hostname);
    }

    //---------------------------------------------------------------------
    // m_connected_run_list
    m_connected_run_list->Init(GetConn(), P2P_CTL_PEER_APP_TYPE_CALL);

    //---------------------------------------------------------------------
    // m_app_call_list
    m_app_call_list->Init(GetConn(), P2P_CTL_PEER_APP_TYPE_CALL);

    //---------------------------------------------------------------------
    // m_app_call_list
    m_app_listen_list->Init(GetConn(), P2P_CTL_PEER_APP_TYPE_LISTEN);

    //---------------------------------------------------------------------
    // m_app_port
    m_app_port->Append("tcp/80 http");
    m_app_port->Append("tcp/21 ftp");
    m_app_port->Append("tcp/22 ssh");
    m_app_port->Append("tcp/139 netbios");
    m_app_port->Append("tcp/1720 h323");
    m_app_port->Append("tcp/5900 vnc");

    //---------------------------------------------------------------------
    // m_account_html
    m_account_html->SetFrame(this);

    ConnectPeer();
}

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME
 */

/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME
 */

void p2puiFrame::OnCloseWindow( wxCloseEvent& event )
{
#if wxHAS_TASKBARICON
    if( event.CanVeto() 
	//&& !event.GetLoggingOff() 
	//&& !event.GetSessionEnding() 
	//&& !event.GetForce()
	) {
	//event.Veto();
	Hide();
    }
    else {
	Destroy();
    }
#else
    Destroy();
#endif // wxHAS_TASKBARICON
    //event.Skip();
}

/*!
 * wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK
 */
void p2puiFrame::OnNotebookPageChanged( wxNotebookEvent& event )
{
    wxNotebookPage *page = m_notebook->GetPage(m_notebook->GetSelection());
    if( !page ) {
	return;
    }

    switch(page->GetId()) {
    case ID_CONNECTED_PANEL:
	if( m_connect_text->GetValue().Length() <= 0 ) {
	    m_connect_text->SetValue("Search for hosts here");
	    m_connect_text->SetSelection(-1, -1);
	    m_connect_text->SetFocus();
	}

	break;
    default:
	break;
    }

    RefreshPage();
    event.Skip();
}

/*!
 * wxEVT_UPDATE_UI event handler for ID_NOTEBOOK
 */
void p2puiFrame::OnNotebookUpdate( wxUpdateUIEvent& event )
{
   if( m_connected_list->GetSelection() < 0 ) {
       m_connected_list->SetSelection(0);
   }

   if( m_connected_run_list->GetListBox()->GetSelection() < 0 ) {
       m_connected_run_list->GetListBox()->SetSelection(0);
   }

   event.Skip();
}

void p2puiFrame::ConnectClient(const char *hostname) {

   do {
       if( !hostname || !*hostname ) {
	   break;
       }

       MruCombo(m_connect_text, m_cfg, "connected_mru", hostname);

       /* either start a search or connect to a host */
       if( p2p_hostname_ok((char*)hostname) ) {
	   /* start connecting to a host by looking up its name */
	   AddConnectedClient(hostname);
	   int i = GetConnectedListBox()->SelectHost(hostname, 1);
	   if( i >=0 ) {
	       GetConnectedListBox()->ScrollToLine(i);
	   }

	   struct hostent *he;
	   he = gethostbyname(hostname);
	   break;
       }

       // create a new search result dialog to catch the result
       p2puiSearchResultDialog *dlg = GetSearchResultDialog(hostname, 1);
       dlg->Search(hostname);
       dlg->Show();
   } while(0);
}


/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_SEARCH_BUTTON
*/
void p2puiFrame::OnConnectButtonClick( wxCommandEvent& event )
{
   ConnectClient(m_connect_text->GetValue());
}

/*!
* wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_CONNECT_TEXT
*/

void p2puiFrame::OnConnectTextSelected( wxCommandEvent& event )
{
   OnConnectButtonClick(event);
   event.Skip();
}

void
p2puiFrame::AddConnectedClient(const char *hostname) {
   m_connected_list->AddHost(hostname);
}


/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DISCONNECT
*/

void p2puiFrame::OnDisconnectClick( wxCommandEvent& event )
{
   int idx;
   unsigned long cookie;

   // send my peer a disconnect message for every selected host
   for(idx=m_connected_list->GetFirstSelected(cookie); idx>=0; idx=m_connected_list->GetNextSelected(cookie)) {
       p2p_ctl_peer_t msg;
       p2p_ctl_peer_init(&msg, P2P_CTL_PEER_CLIENT_STATUS);
       {
	   p2p_ctl_peer_client_status_t *m = 
	       &msg.p2p_ctl_peer_t_u.client_status;

	   m->hostname = (char*)m_connected_list->GetHost(idx).c_str();
	   m->ui_state = P2P_UI_CONNECT_CLIENT_CLOSED;
       }
       m_conn.Send(&msg);
   }

   /* delete selected entries from m_connected_list */
   wxArrayString a;
   for(idx=(int)m_connected_list->GetItemCount()-1; idx>=0; idx--) {
       if( m_connected_list->IsSelected(idx) ) {
	   m_connected_list->RemoveHostAt(idx);
       }
   }

   event.Skip();
}

/*!
* wxEVT_UPDATE_UI event handler for ID_DISCONNECT
*/

void p2puiFrame::OnDisconnectUpdate( wxUpdateUIEvent& event )
{
   bool enable = m_connected_list->GetSelectedCount()>0 ? TRUE : FALSE;
   m_disconnect_button->Enable(enable);
   event.Skip();
}


void p2puiFrame::SetAutoUpdate(int val)
{
   m_update->SetAutoUpdate(val);
}


void p2puiFrame::CheckForUpdates()
{
   m_update->StartUpdate(1);
}


void p2puiFrame::Shutdown() {
   int i;
   p2p_ctl_peer_t msg;

   do {
       i = wxMessageBox("Are you sure you want to close all connections"
			" to the " P2PVPN_PRODUCT " network?", 
			"Shut down " P2PVPN_PRODUCT " Network", 
			wxOK | wxCANCEL | wxICON_QUESTION,
			this);
       if( i != wxOK ) {
	   break;
       }

       m_shutdown = 1;
       p2p_ctl_peer_init(&msg, P2P_CTL_PEER_STATUS);
       {
	   p2p_ctl_peer_client_status_t *m = 
	       &msg.p2p_ctl_peer_t_u.client_status;
	   m->ui_state = P2P_UI_CONNECT_CLIENT_CLOSED;
       }
       m_conn.Send(&msg);

   } while(0);
}


void p2puiFrame::OnLoginConnectListSelected( wxListEvent& event )
{
   wxString hostname = m_login_connect_list->GetItemText(event.GetIndex());
   wxString username;
   wxString password;

   p2puiCtlPeerMsg *uimsg = m_conn.GetUserInfoConnect(hostname);
   if( uimsg ) {
       username = uimsg->GetUserInfo()->username;
       password = "";
   }

   m_login_hostname->SetValue(hostname);
   m_login_username->SetValue(username);
   m_login_password->SetValue(password);
   m_login_confirm->SetValue(password);
   event.Skip();

   event.Skip();
}

/*!
* wxEVT_COMMAND_LIST_DELETE_ITEM event handler for ID_LOGIN_CONNECT_LIST
*/

void p2puiFrame::OnLoginConnectListDeleteItem( wxListEvent& event )
{
////@begin wxEVT_COMMAND_LIST_DELETE_ITEM event handler for ID_LOGIN_CONNECT_LIST in p2puiFrame.
   // Before editing this code, remove the block markers.
   event.Skip();
////@end wxEVT_COMMAND_LIST_DELETE_ITEM event handler for ID_LOGIN_CONNECT_LIST in p2puiFrame. 
}

/*!
* Should we show tooltips?
*/

bool p2puiFrame::ShowToolTips()
{
   return TRUE;
}

/*!
* Get bitmap resources
*/

wxBitmap p2puiFrame::GetBitmapResource( const wxString& name )
{
   // Bitmap retrieval
////@begin p2puiFrame bitmap retrieval
   return wxNullBitmap;
////@end p2puiFrame bitmap retrieval
}

/*!
* Get icon resources
*/

wxIcon p2puiFrame::GetIconResource( const wxString& name )
{
   // Icon retrieval
////@begin p2puiFrame icon retrieval
   return wxNullIcon;
////@end p2puiFrame icon retrieval
}

p2puiSearchResultDialog*
p2puiFrame::GetSearchResultDialog(const char *search_string, int create) {
   p2puiSearchResultDialog *dlg;
   int i;
   for(i = m_search_dialogs.Count()-1; i>=0; i--) {
       // always reuse the first dialog
       break;
   }
   if( i >=0 ) {
       dlg = m_search_dialogs[i];
   }
   else if( create ) {
       dlg = new p2puiSearchResultDialog(this, search_string);
       m_search_dialogs.Add(dlg);
   }
   return dlg;
}

void
p2puiFrame::DestroySearchResultDialog(p2puiSearchResultDialog *dlg) {
   int i;
   for(i = m_search_dialogs.Count()-1; i>=0; i--) {
       if( m_search_dialogs[i] == dlg ) {
	   m_search_dialogs.RemoveAt(i);
       }
   }
   dlg->Destroy();
}

p2puiCtlPeerConn*
p2puiFrame::GetConn() {
   return &m_conn;
}



void p2puiFrame::OnConnectedListClick( wxCommandEvent& event )
{
   unsigned long cookie;
   int idx;
   idx = m_connected_list->GetFirstSelected(cookie);
   if( idx >=0 ) {
       wxString hostname = m_connected_list->GetHost(idx);
       m_connect_text->SetValue(hostname);
       m_connect_text->SetSelection(-1,-1);
    }

    event.Skip();
}

void p2puiFrame::OnConnectedListDClick( wxCommandEvent& event )
{
   int idx;
   unsigned long cookie;
   p2puiClientDialog *dlg = 0;
   for(idx=m_connected_list->GetFirstSelected(cookie); idx>=0; idx=m_connected_list->GetNextSelected(cookie)) {
       wxString lhostname;
       lhostname = m_connected_list->GetHost(idx);
       lhostname.MakeLower();
       dlg = m_client_dialog[lhostname];
       if( dlg ) {
	   dlg->Show();
       }
       else {
	   int app_idx = m_connected_run_list->GetListBox()->GetSelection();
	   if( app_idx >= 0 ) {
	       wxString app_name = 
		   m_connected_run_list->GetAppNameList()[app_idx];
	       p2puiCtlPeerMsg *app_info = m_conn.GetAppInfo(app_name);
	       if( app_info ) {
		   m_conn.Run(app_info->GetAppInfo()->cmdline, lhostname);
	       }
	   }
       }
   }
   event.Skip();
}


/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ACCOUNT_CONFIGURE
*/

void p2puiFrame::OnAccountConfigureClick( wxCommandEvent& event )
   {
   if( !m_account_dialog ) {
       m_account_dialog = new p2puiAccountDialog(this);
   }
   m_account_dialog->Show();
   event.Skip();
}

p2puiUpdateDialog*
p2puiFrame::GetUpdateDialog() {
   return m_update;
}

/*!
* wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FILE_CLOSE
*/

void p2puiFrame::OnMenuFileCloseClick( wxCommandEvent& event )
{
   event.Skip();

   wxString s;
   s << 
       "This application controls the " P2PVPN_PRODUCT " service that is"
       " running in the background.  If you quit this control panel, " P2PVPN_PRODUCT " will continue to run, but you will not be prompted to"
       " accept incoming passwords or supply outgoing passwords for new"
       " connections.  It's better to hide this control panel and it will run in your"
       " system tray and prompt you when required."
       "\r\n Click YES to close this control panel"
       "\r\n Click NO to hide this control panel and continue to run in your system tray"
       "\r\n Click Cancel to return to the control panel"
       ;
   int i;
   i = wxMessageBox(s, "Quit " P2PVPN_PRODUCT " Control Panel?", wxYES_NO | wxCANCEL, this);
   if( i == wxYES ) {
       Destroy();
   }
   else if( i == wxNO ) {
       Hide();
   }


}


/*!
* wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FILE_SHUTDOWN
*/
void p2puiFrame::OnMenuFileShutdownClick( wxCommandEvent& event )
{
   event.Skip();
   Shutdown();
}


/*!
* wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for ID_LOGIN_ACCEPT_LIST
*/

void p2puiFrame::OnLoginAcceptListSelected( wxListEvent& event )
{
   wxString username = m_login_accept_list->GetItemText(event.GetIndex());
   wxString password;
   p2puiCtlPeerMsg *uimsg = m_conn.GetUserInfoAccept(username);
   if( uimsg ) {
       password = "";
   }

   m_login_hostname->SetValue("");
   m_login_username->SetValue(username);
   m_login_password->SetValue(password);
   m_login_confirm->SetValue(password);
   event.Skip();
}

/*!
* wxEVT_COMMAND_LIST_DELETE_ITEM event handler for ID_LOGIN_ACCEPT_LIST
*/

void p2puiFrame::OnLoginAcceptListDeleteItem( wxListEvent& event )
{
////@begin wxEVT_COMMAND_LIST_DELETE_ITEM event handler for ID_LOGIN_ACCEPT_LIST in p2puiFrame.
   // Before editing this code, remove the block markers.
   event.Skip();
////@end wxEVT_COMMAND_LIST_DELETE_ITEM event handler for ID_LOGIN_ACCEPT_LIST in p2puiFrame. 
}


wxString p2puiFrame::GetPeerStatusHtml() {
   return m_account_html->ToText();
}

void p2puiFrame::SetPeerStatus(p2p_ctl_peer_status_t *m) {
   int show=0;
   wxString img, msg, html;
   int i;

   do {
       if( !m ) {
	   m = m_conn.GetPeerStatus();
       }
       assertb(m);


       switch(m->ui_state) {
       case P2P_PEER_UI_PASSWORD_MISSING:
       case P2P_PEER_UI_HOSTNAME_MISSING:
	   cfg_get_int(m_cfg, "ui/no_register", &i);
	   show = !i;
	   break;

       case P2P_PEER_UI_LOGIN_REJECTED:
	   show = 1;
	   break;

       default:
	   break;
       }

       m_account_html->SetPage(m_conn.GetPeerStatusHtml(m, 1));
       if( show ) {
	   if( !m_account_dialog ) {
	       m_account_dialog = new p2puiAccountDialog(this);
	   }
	   m_account_dialog->Show();
       }

       m_login_accept_existing_check->SetValue(m->opts & P2P_PEER_OPT_CLIENT_LOGIN_LOCAL_ACCOUNTS ? TRUE : FALSE);
       m_login_accept_any_check->SetValue(m->opts & P2P_PEER_OPT_CLIENT_PKT_ALLOW_ANONYMOUS ? TRUE : FALSE);
       //m_login_opts_dirty = 0;

   } while(0);
}
/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
*/

void p2puiFrame::OnLoginSaveClick( wxCommandEvent& event )
{
   p2p_ctl_peer_t msg;
   wxString hostname = m_login_hostname->GetValue();
   wxString username = m_login_username->GetValue();
   wxString password = m_login_password->GetValue();

   if( !ConfirmPassword(m_login_password, m_login_confirm) ) {
       return;
   }

   p2p_ctl_peer_init(&msg, P2P_CTL_PEER_USER_INFO); 
   {
       p2p_ctl_peer_user_info_t *m = &msg.p2p_ctl_peer_t_u.user_info;
       m->hostname = (char*)hostname.c_str();
       m->username = (char*)username.c_str();
       m->password_md5 = (char*)password.c_str();
       m->last_from = "";
   }
   m_conn.Send(&msg);

   /* get a new user list */
   p2p_ctl_peer_init(&msg, P2P_CTL_PEER_LIST_GET);
   msg.p2p_ctl_peer_t_u.list_get.type = P2P_CTL_PEER_USER_INFO;
   m_conn.Send(&msg);

   event.Skip();
}

/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ACCEPT_DELETE
*/

void p2puiFrame::OnLoginDeleteClick( wxCommandEvent& event )
{
   p2p_ctl_peer_t msg;

   if( wxMessageBox("Do you really want to delete this password?",
		    "Confirm Delete",
		    wxICON_QUESTION | wxYES_NO
		    ) != wxYES ) {
       return;
   }

   wxString hostname = m_login_hostname->GetLineText(0);
   wxString username = m_login_username->GetLineText(0);

   p2p_ctl_peer_init(&msg, P2P_CTL_PEER_USER_INFO);
   {
       p2p_ctl_peer_user_info_t *m = &msg.p2p_ctl_peer_t_u.user_info;
       m->hostname = (char*)hostname.c_str();
       m->username = (char*)username.c_str();
       m->password_md5 = "";
       m->last_from = "";
   }
   m_conn.Send(&msg);

   OnLoginClearClick(event);

   /* get a new user list */
   p2p_ctl_peer_init(&msg, P2P_CTL_PEER_LIST_GET);
   msg.p2p_ctl_peer_t_u.list_get.type = P2P_CTL_PEER_USER_INFO;
   m_conn.Send(&msg);

   event.Skip();
}

/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ACCEPT_CANCEL
*/

void p2puiFrame::OnLoginClearClick( wxCommandEvent& event )
{
   m_login_hostname->SetValue("");
   m_login_username->SetValue("");
   m_login_password->SetValue("");
   m_login_confirm->SetValue("");
   event.Skip();
}


/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON
*/

void p2puiFrame::OnLoginOptionsSaveClick( wxCommandEvent& event )
{
   p2p_ctl_peer_t msg;
   p2p_ctl_peer_init(&msg, P2P_CTL_PEER_STATUS);
   {
       p2p_ctl_peer_status_t *m = &msg.p2p_ctl_peer_t_u.peer_status;
       *m = *m_conn.GetPeerStatus();

       m->opts_mask = 0;
       m->opts_mask |= P2P_PEER_OPT_CLIENT_LOGIN_LOCAL_ACCOUNTS;
       if( m_login_accept_existing_check->IsChecked() ) {
	   m->opts |= P2P_PEER_OPT_CLIENT_LOGIN_LOCAL_ACCOUNTS;
       }
       else {
	   m->opts &= ~P2P_PEER_OPT_CLIENT_LOGIN_LOCAL_ACCOUNTS;
       }

       m->opts_mask |= P2P_PEER_OPT_CLIENT_PKT_ALLOW_ANONYMOUS;
       if( m_login_accept_any_check->IsChecked() ) {
	   m->opts |= P2P_PEER_OPT_CLIENT_PKT_ALLOW_ANONYMOUS;
       }
       else {
	   m->opts &= ~P2P_PEER_OPT_CLIENT_PKT_ALLOW_ANONYMOUS;
       }
   }
   m_conn.Send(&msg);
   m_login_options_dirty = 0;

   event.Skip();
}




/*!
* wxEVT_COMMAND_TEXT_UPDATED event handler for ID_LOGIN_HOSTNAME
*/

void p2puiFrame::OnLoginHostnameUpdated( wxCommandEvent& event )
{
   event.Skip();
}


/*!
* wxEVT_UPDATE_UI event handler for ID_LOGIN_SAVE
*/

void p2puiFrame::OnLoginSaveUpdate( wxUpdateUIEvent& event )
{
   bool is_connect = m_login_hostname->GetValue().Length()>0;
   bool is_accept = !is_connect && m_login_username->GetValue().Length()>0;
   m_login_connect_radio->SetValue(is_connect);
   m_login_accept_radio->SetValue(is_accept);
   m_login_save->Enable(is_connect || is_accept);
   event.Skip();
}


/*!
* wxEVT_UPDATE_UI event handler for ID_LOGIN_OPTIONS_SAVE
*/

void p2puiFrame::OnLoginOptionsSaveUpdate( wxUpdateUIEvent& event )
{
   m_login_options_save->Enable(m_login_options_dirty ? TRUE : FALSE);
   event.Skip();
}


/*!
* wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_LOGIN_ACCEPT_LOCAL
*/

void p2puiFrame::OnLoginAcceptLocalClick( wxCommandEvent& event )
{
   m_login_options_dirty = 1;
   event.Skip();
}


/*!
* wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_LOGIN_ACCEPT_ANY
*/

void p2puiFrame::OnLoginAcceptAnyClick( wxCommandEvent& event )
{
   m_login_options_dirty = 1;
   event.Skip();
}


/*!
* wxEVT_UPDATE_UI event handler for ID_PORT_CMDLINE
*/

void p2puiFrame::OnAppCmdlineUpdate( wxUpdateUIEvent& event )
{
////@begin wxEVT_UPDATE_UI event handler for ID_PORT_CMDLINE in p2puiFrame.
   // Before editing this code, remove the block markers.
   event.Skip();
////@end wxEVT_UPDATE_UI event handler for ID_PORT_CMDLINE in p2puiFrame. 
}

/*!
* wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_PORT_AUTORUN
*/

void p2puiFrame::OnAppAutorunClick( wxCommandEvent& event )
{
////@begin wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_PORT_AUTORUN in p2puiFrame.
   // Before editing this code, remove the block markers.
   event.Skip();
////@end wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_PORT_AUTORUN in p2puiFrame. 
}

/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PORT_SAVE
*/

void p2puiFrame::OnAppSaveClick( wxCommandEvent& event )
{
   wxString cmdline = m_app_cmdline->GetValue();
   wxString label = m_app_label->GetValue();
   wxString port = m_app_port->GetValue();
   //wxString desc = m_app_desc->GetValue();
   char img_buf[16384];
   size_t img_len;

   if( port.Length() && !p2p_ctl_peer_app_info_port_ok((char*)port.c_str()) ) {
       wxMessageBox("The application port name must contain the protocol (tcp or udp) and the numeric port number like tcp/xxx ot udp/xxx", "Invalid Application Port Name", wxOK);
       m_app_port->SetSelection(-1, -1);
       m_app_port->SetFocus();
   }

   p2puiCtlPeerMsg *uimsg = m_conn.GetAppInfo(m_app_info_name);

   p2p_ctl_peer_t msg;
   p2p_ctl_peer_init(&msg, P2P_CTL_PEER_APP_INFO);
   {
       p2p_ctl_peer_app_info_t *m = &msg.p2p_ctl_peer_t_u.app_info;
       p2p_ctl_peer_app_info_t *info = uimsg ? uimsg->GetAppInfo() : 0;

       if( info ) {
	   *m = *(uimsg->GetAppInfo());
       }

       m->name = (char*)m_app_info_name.c_str();
       m->cmdline = (char*)cmdline.c_str();
       m->label = (char*)label.c_str();
       m->port = (char*)port.c_str();
       m->type = port.Length() > 0 
	   ? P2P_CTL_PEER_APP_TYPE_LISTEN 
	   : P2P_CTL_PEER_APP_TYPE_CALL;

       // clear m_app_info_name if the app changed from a listener to
       // a caller
       if( !info || m->type != info->type ) {
	   m_app_info_name = "";
	   m->name = "";
       }

       wxMemoryOutputStream os;
       m_app_icon->GetBitmapLabel().ConvertToImage().SaveFile(os, wxBITMAP_TYPE_PNG);
       img_len = os.CopyTo(img_buf, sizeof(img_buf));
       m->icon_png.icon_png_val = img_buf;
       m->icon_png.icon_png_len = img_len;

       m->allow_public = port.Length() > 0 && m_app_allow_public->IsChecked();
       m->autorun = port.Length() > 0 && m_app_autorun->IsChecked();
       m->delete_me = 0;
   }
   m_conn.Send(&msg);
   event.Skip();
}

/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PORT_DELETE
*/

void p2puiFrame::OnAppDeleteClick( wxCommandEvent& event )
{
   if(  m_app_info_name.Length() <= 0 ) {
       return;
   }

   p2puiCtlPeerMsg *uimsg = m_conn.GetAppInfo(m_app_info_name);


   p2p_ctl_peer_t msg;
   p2p_ctl_peer_init(&msg, P2P_CTL_PEER_APP_INFO);
   {
       p2p_ctl_peer_app_info_t *m = &msg.p2p_ctl_peer_t_u.app_info;
       if( uimsg ) {
	   *m = *(uimsg->GetAppInfo());
       }
       m->name = (char*)m_app_info_name.c_str();
       m->delete_me = 1;
   }
   m_conn.Send(&msg);
   event.Skip();
}

/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_PORT_RUN
*/

void p2puiFrame::OnAppRunClick( wxCommandEvent& event )
{
   p2puiCtlPeerMsg *uimsg = m_conn.GetAppInfo(m_app_info_name);
   p2p_ctl_peer_app_info_t *info = uimsg ? uimsg->GetAppInfo() : 0;
   if( info ) {
       proc_shellex(0, info->cmdline, 0);
   }
   event.Skip();
}



/*!
* wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENUFILE_HIDE
*/

void p2puiFrame::OnMenuFileHideClick( wxCommandEvent& event )
{
   Hide();
   event.Skip();
}

void
p2puiFrame::SetAppEditInfo(p2p_ctl_peer_app_info_t *info) {
   m_app_info_name = info ? info->name : "";
   m_app_cmdline->SetValue(info ? info->cmdline : "");
   //m_app_desc->SetValue(info ? info->desc : "");
   m_app_label->SetValue(info ? info->label : "");
   int got_icon = 0;
   if( info && info->icon_png.icon_png_len ) {
       wxImage img;
       wxMemoryInputStream is(info->icon_png.icon_png_val,
			      info->icon_png.icon_png_len);
       if( img.LoadFile(is, wxBITMAP_TYPE_PNG) ) {
	   wxBitmap bmp(img);
	   m_app_icon->SetBitmapLabel(bmp);
	   got_icon = 1;
       }
   }
   if( !got_icon ) {
       m_app_icon->SetBitmapLabel(wxNullBitmap);
   }
   m_app_port->SetValue(info ? info->port : "");
   wxString port = m_app_port->GetValue();
   m_app_port_panel->Enable(port.Length()>0 ? TRUE : FALSE);
   m_app_allow_public->SetValue(port.Length()>0 && info && info->allow_public ? TRUE : FALSE);
   m_app_autorun->SetValue(port.Length()>0 && info && info->autorun ? TRUE : FALSE);
}

void
p2puiFrame::OnAppListenListClick( wxCommandEvent& event ) {
   p2puiCtlPeerMsg *uimsg;
   int idx;
   do {
       idx = m_app_listen_list->GetListBox()->GetSelection();
       assertb(idx>=0 && idx<(int)m_app_listen_list->GetListBox()->GetItemCount());
       m_app_call_list->GetListBox()->SetSelection(-1);
       uimsg = m_conn.GetAppInfo(m_app_listen_list->GetAppNameList()[idx]);
       assertb(uimsg);
       SetAppEditInfo(uimsg->GetAppInfo());
   } while(0);
   event.Skip();
}

void
p2puiFrame::OnAppCallListClick( wxCommandEvent& event ) {
   p2puiCtlPeerMsg *uimsg;
   int idx;
   do {
       idx = m_app_call_list->GetListBox()->GetSelection();
       assertb(idx>=0 && idx<(int)m_app_call_list->GetListBox()->GetItemCount());
       m_app_listen_list->GetListBox()->SetSelection(-1);
       uimsg = m_conn.GetAppInfo(m_app_call_list->GetAppNameList()[idx]);
       assertb(uimsg);
       SetAppEditInfo(uimsg->GetAppInfo());
   } while(0);
   event.Skip();
}

/*!
* wxEVT_UPDATE_UI event handler for ID_APP_LABEL
*/

void p2puiFrame::OnAppLabelUpdate( wxUpdateUIEvent& event )
{
////@begin wxEVT_UPDATE_UI event handler for ID_APP_LABEL in p2puiFrame.
   // Before editing this code, remove the block markers.
   event.Skip();
////@end wxEVT_UPDATE_UI event handler for ID_APP_LABEL in p2puiFrame. 
}


/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_APP_NEW
*/

void p2puiFrame::OnAppNewClick( wxCommandEvent& event )
{
   SetAppEditInfo(0);
   event.Skip();
}


/*!
* wxEVT_COMMAND_TEXT_UPDATED event handler for ID_APP_PORT
*/
void p2puiFrame::OnAppPortUpdated( wxCommandEvent& event )
{
   wxString port = m_app_port->GetValue();
   m_app_port_panel->Enable(port.Length()>0 ? TRUE : FALSE);
   event.Skip();
}

void
p2puiFrame::UpdateAppIcon(const char *path_in) {
   wxString path;
   char buf[4096], *p;

   if( path_in ) {
       path = path_in;
   }
   else {
       path = m_app_cmdline->GetValue();
   }
   p = (char*)path.c_str();
   opt_quoted_dec(&p, buf, sizeof(buf), 1);

#ifdef __WIN32__
   SHFILEINFO shfi;
   SHGetFileInfo(buf, 0, &shfi, sizeof(shfi), SHGFI_ICON | SHGFI_LARGEICON);

   wxIcon icon;
   wxSize size = wxGetHiconSize(shfi.hIcon);
   icon.SetSize(size.x, size.y);
   icon.SetHICON((WXHICON)shfi.hIcon);
   m_app_icon->SetBitmapLabel(icon);
#endif
}


/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_APP_BROWSE
*/

void p2puiFrame::OnAppBrowseClick( wxCommandEvent& event )
{
   wxFileDialog dlg(this, "Choose an application", "", "", 
		    "EXE Files|*.exe|All Files|*.*", 
		    wxOPEN | wxFILE_MUST_EXIST);

   if( dlg.ShowModal() != wxID_OK ) {
       return;
   }

   wxString path = dlg.GetPath();
   if( path.Contains(" ") ) {
       path.Prepend("\"");
       path.Append("\"");
   }
   if( m_app_port->GetValue().Length()<=0 ) {
       path.Append(" %h");
   }

   m_app_cmdline->SetValue(path);
   UpdateAppIcon();

   if( m_app_label->GetValue().Length() <= 0 ) {
       char buf[4096], *p;
       path = dlg.GetPath();
       dir_basename((char*)path.c_str(), buf, sizeof(buf));
       p = strrchr(buf, '.');
       if( p ) {
	   *p = 0;
       }
       m_app_label->SetValue(p);
   }


   event.Skip();
}


/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_APP_ICON
*/

void p2puiFrame::OnAppIconClick( wxCommandEvent& event )
{
   wxFileDialog dlg(this, "Choose an icon", "", "", 
		    "Icon Files|*.ico,*.png,*.ico,*.gif,*.exe,*.dll|All Files|*.*", wxOPEN | wxFILE_MUST_EXIST);

   if( dlg.ShowModal() != wxID_OK ) {
       return;
   }

   wxImage img;
   wxString path = dlg.GetPath();
   //if( path.endswidth("*.dll","*.exe") ) {
   //}
   if( img.LoadFile(path) ) {
       m_app_icon->SetBitmapLabel(img);
   }
   else {
       // img/32/client_unknown.png;
       //m_app_icon.SetBitmapLabel(img);
   }


   event.Skip();
}


/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_APP_ICON_UPDATE
*/

void p2puiFrame::OnAppIconUpdateClick( wxCommandEvent& event )
{
   UpdateAppIcon();
   event.Skip();
}


/*!
* wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CONNECT_RUN_BUTTON
*/

void p2puiFrame::OnConnectRunButtonClick( wxCommandEvent& event )
{
   OnConnectedListDClick(event);
   event.Skip();
}


/*!
* wxEVT_UPDATE_UI event handler for ID_CONNECT_RUN_BUTTON
*/

void p2puiFrame::OnConnectRunButtonUpdate( wxUpdateUIEvent& event )
{
   bool enable = TRUE;

   enable = m_connected_list->GetSelectedCount()>0
       && m_connected_run_list->GetListBox()->GetSelectedCount() > 0
       ;
   m_connected_run_button->Enable(enable);

   event.Skip();
}


/*!
* wxEVT_UPDATE_UI event handler for ID_CONNECT_BUTTON
*/

void p2puiFrame::OnConnectButtonUpdate( wxUpdateUIEvent& event )
{
   //m_connect_button->Enable(m_connect_text && m_connect_text->GetValue().Length()>0);
   event.Skip();
}


/*!
* wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FILE_UPDATES
*/

void p2puiFrame::OnMenuFileUpdatesClick( wxCommandEvent& event )
{
   CheckForUpdates();
   event.Skip();
}


p2puiHostListBox*
p2puiFrame::GetConnectedListBox() {
   return m_connected_list;
}

/*!
* wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_HELP_ABOUT
*/

void p2puiFrame::OnMenuHelpAboutClick( wxCommandEvent& event )
{
   OnAbout(event);
   event.Skip();
}


/*!
* wxEVT_RIGHT_UP event handler for ID_CONNECTED_LIST
*/

void p2puiFrame::OnConnectedListRightClick( wxCommandEvent& event )
{
   int i, j, idx;
   wxString clip_text;
   p2puiCtlPeerMsg *app_info=0;
   p2puiCtlPeerMsg *client_status=0;
   bkPopupMenu *menu = 0;
   wxMenuItem *mit;
   wxString s;
   wxPoint pos;
   wxArrayString app_list;

   enum {
       ID_CONNECTED_MENU_FIRST = bkPopupMenu::MENU_ID_FIRST
       ,ID_CONNECTED_MENU_NONE = ID_CONNECTED_MENU_FIRST
       ,ID_CONNECTED_MENU_COPY_HOSTNAME =  ID_CONNECTED_MENU_FIRST+1
       ,ID_CONNECTED_MENU_COPY_IP = ID_CONNECTED_MENU_FIRST+2
       ,ID_CONNECTED_MENU_RUN = ID_CONNECTED_MENU_FIRST+3
   };

   do {
       // get selected client
       idx = event.GetInt();
       s = m_connected_list->GetHost(idx);
       client_status = m_conn.GetClientStatus(s);
       if( !client_status ) break;

       // select this hostname in the list and in my connect text
       if( !m_connected_list->IsSelected(idx) ) {
	   if( m_connected_list->HasMultipleSelection() ) {
	       for(i=0; i<(int)m_connected_list->GetItemCount(); i++) {
		   m_connected_list->Select(i, i==idx);
	       }
	   }
	   m_connected_list->SetSelection(idx);
       }
       m_connect_text->SetValue(client_status->GetVal("hostname"));
       m_connect_text->SetSelection(-1,-1);

       // make menu
       menu = new bkPopupMenu();
       assertb(menu);

       // title
       s = "";
       s << "Connected to " << client_status->GetVal("hostname");
       menu->SetTitle(s);

       wxFont font;
       font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
       font.SetWeight(wxBOLD);

       // copy info to clipboard
       mit = new wxMenuItem(menu, ID_CONNECTED_MENU_NONE, 
			    "Copy to Clipboard...");
       assertb(mit);
#ifdef __WIN32__
       mit->SetFont(font);
#endif // __WIN32__
       menu->Append(mit);

       menu->Append(ID_CONNECTED_MENU_COPY_HOSTNAME, 
		    client_status->GetVal("hostname"));
       menu->Append(ID_CONNECTED_MENU_COPY_IP, 
		    client_status->GetVal("pseudo_ipaddr"));


       // choose an application to run
       menu->AppendSeparator();

       mit = new wxMenuItem(menu, ID_CONNECTED_MENU_NONE, 
			    "Run Application...");
       assertb(mit);
#ifdef __WIN32__
       mit->SetFont(font);
#endif // __WIN32__
       menu->Append(mit);

       m_conn.GetAppNameArray(&app_list);
       for(i=0; i<(int)app_list.Count()-1; i++) {
	   app_info = m_conn.GetAppInfo(app_list[i]);
	   p2p_ctl_peer_app_info_t *info = app_info->GetAppInfo();
	   assertb(info);

	   if( info->type != P2P_CTL_PEER_APP_TYPE_CALL
	       || info->delete_me ) {
	       continue;
	   }


	   wxMenuItem *it = new wxMenuItem(menu, 
					  ID_CONNECTED_MENU_RUN+i,
					  info->label);
	   if( info->icon_png.icon_png_len > 0 ) {
	       wxMemoryInputStream is(info->icon_png.icon_png_val,
				      info->icon_png.icon_png_len);
	       wxImage img;
	       if( img.LoadFile(is, wxBITMAP_TYPE_PNG) ) {
		   wxBitmap bmp(img);
		   it->SetBitmap(bmp);
	       }
	   }
	   menu->Append(it);
       }

       PopupMenu(menu);

       i = menu->GetClicked();
       if( i >= ID_CONNECTED_MENU_RUN ) {
	   /* run the app */
	   i -= ID_CONNECTED_MENU_RUN;
	   if( i < (int)app_list.Count() ) {
	       m_conn.Run(m_conn.GetAppInfo(app_list[i])->GetAppInfo()->cmdline, 
			  client_status->GetVal("hostname"));

	       for(j=0; j<(int)m_connected_run_list->GetAppNameList().Count(); j++) {
		   if( app_list[i].CmpNoCase(m_connected_run_list->GetAppNameList()[j]) == 0 ) {
		       m_connected_run_list->GetListBox()->SetSelection(j);
		       break;
		   }
	       }
	   }
       }
       else {
	   switch(i) {
	   case ID_CONNECTED_MENU_COPY_HOSTNAME:
	       clip_text << client_status->GetVal("hostname");
	       break;
	   case ID_CONNECTED_MENU_COPY_IP:
	       clip_text << client_status->GetVal("pseudo_ipaddr");
	       break;
	   }
	   if( clip_text.Length() > 0 ) {
	       if( wxTheClipboard->Open() ) {
		   wxTheClipboard->SetData(new wxTextDataObject(clip_text));
		   wxTheClipboard->Close();
	       }
	   }
       }
   } while(0);
   if( menu ) {
       delete menu;
   }

   event.Skip();
}


/*!
* wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_CONFIGURE_ACCOUNT
*/

void p2puiFrame::OnMenuConfigureAccountClick( wxCommandEvent& event )
{
   OnAccountConfigureClick(event);
   event.Skip();
}


/*!
* wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_HELP_CONTENTS
*/

void p2puiFrame::OnMenuHelpContentsClick( wxCommandEvent& event )
{
   ShowHelp();
   event.Skip();
}


