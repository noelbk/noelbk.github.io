/////////////////////////////////////////////////////////////////////////////
// Name:        p2p_ui_frame.h
// Purpose:     
// Author:      Noel Burton-Krahn
// Modified by: 
// Created:     08/17/04 00:48:26
// RCS-ID:      
// Copyright:   Copyright Burton-Krahn Inc
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _P2P_UI_FRAME_H_
#define _P2P_UI_FRAME_H_

#ifdef __GNUG__
#pragma interface "p2p_ui_frame.cpp"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wx/frame.h"
#include "wx/notebook.h"
#include "wx/html/htmlwin.h"
#include "wx/splitter.h"
#include "wx/listctrl.h"
////@end includes

#include "wx/datetime.h"
#include "wx/listctrl.h"
#include "wx/hashmap.h"
#include "wx/notebook.h"
#include "wx/taskbar.h"
#include "wx/checkbox.h"

#include "wx/combobox.h"
#include "wx/button.h"
#include "wx/radiobut.h"
#include "wx/statbmp.h"
#include "wx/taskbar.h"
#include "wx/frame.h"
#include "wxconfig.h"
#include "wx/textctrl.h"
#include "wx/radiobox.h"
#include "wx/stattext.h"
#include "wx/statbox.h"
#include "wx/sizer.h"
#include "wx/htmllbox.h"

#include "p2p_ui_ctl_peer_conn.h"
#include "p2p_ui_client_dialog.h"
#include "p2p_ui_update_dialog.h"
#include "p2p_ui_account_dialog.h"
#include "p2p_ui_search_result_dialog.h"

/*!
 * Forward declarations
 */

////@begin forward declarations
class wxNotebook;
class p2puiAccountHtmlWindow;
class KeyComboBox;
class p2puiHostListBox;
class p2puiApplicationListPanel;
class wxHtmlWindow;
class wxListCtrl;
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_FRAME 10000
#define SYMBOL_P2PUIFRAME_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX|wxTAB_TRAVERSAL
#define SYMBOL_P2PUIFRAME_TITLE _("P2P VPN Control Panel")
#define SYMBOL_P2PUIFRAME_IDNAME ID_FRAME
#define SYMBOL_P2PUIFRAME_SIZE wxSize(300, 400)
#define SYMBOL_P2PUIFRAME_POSITION wxDefaultPosition
#define ID_NOTEBOOK 10001
#define ID_ACCOUNT_PANEL 10002
#define ID_FRAME_ACCOUNT_HTML 10012
#define ID_ACCOUNT_CONFIGURE 10016
#define ID_CONNECTED_PANEL 10005
#define ID_SPLITTERWINDOW 10008
#define ID_CONNECTED_CLIENT_PANEL 10031
#define ID_CONNECT_BUTTON 10035
#define ID_CONNECT_TEXT 10036
#define ID_CONNECTED_LIST 10024
#define ID_DISCONNECT 10030
#define ID_CONNECTED_RUN_PANEL 10014
#define ID_CONNECTED_RUN_LIST 10018
#define ID_CONNECT_RUN_BUTTON 10046
#define ID_CONNECT_HTML_HELP 10032
#define ID_LOGIN_PANEL 10003
#define ID_SPLITTERWINDOW1 10037
#define ID_PANEL 10038
#define ID_LOGIN_ACCEPT_LIST 10004
#define ID_LOGIN_CONNECT_LIST 10026
#define ID_PANEL1 10034
#define ID_ACCEPT_PASSWORD_BOX 10040
#define ID_LOGIN_ACCEPT_RADIO 10007
#define ID_LOGIN_CONNECT_RADIO 10033
#define ID_LOGIN_HOSTNAME 10039
#define ID_LOGIN_USERNAME 10009
#define ID_LOGIN_PASSWORD 10011
#define ID_LOGIN_CONFIRM 10013
#define ID_LOGIN_SAVE 10015
#define ID_LOGIN_CLEAR 10025
#define ID_LOGIN_DELETE 10020
#define ID_LOGIN_ACCEPT_LOCAL 10021
#define ID_LOGIN_ACCEPT_ANY 10028
#define ID_LOGIN_OPTIONS_SAVE 10027
#define ID_LOGIN_HTML_HELP 10006
#define ID_APP_PANEL 10048
#define ID_SPLITTERWINDOW2 10043
#define ID_PANEL2 10044
#define ID_APP_CALL_LIST 10059
#define ID_APP_LISTEN_LIST 10042
#define ID_PANEL3 10041
#define ID_APP_LABEL 10061
#define ID_APP_ICON 10063
#define ID_APP_CMDLINE 10045
#define ID_APP_BROWSE 10049
#define ID_APP_ICON_UPDATE 10051
#define ID_APP_PORT 10052
#define ID_APP_PORT_PANEL 10060
#define ID_APP_PUBLIC 10053
#define ID_APP_AUTORUN 10054
#define ID_APP_SAVE 10055
#define ID_APP_RUN 10056
#define ID_APP_NEW 10062
#define ID_APP_DELETE 10050
#define ID_MENU_FILE_UPDATES 10022
#define ID_MENU_FILE_SHUTDOWN 10023
#define ID_MENU_FILE_CLOSE 10017
#define ID_MENU_FILE_HIDE 10019
#define ID_MENU_CONFIGURE_ACCOUNT 10068
#define ID_MENU_HELP_CONTENTS 10010
#define ID_MENU_HELP_ABOUT 10057
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * p2puiFrame class declaration
 */

WX_DEFINE_ARRAY(p2puiSearchResultDialog*, p2puiSearchResultDialogArray);
WX_DECLARE_STRING_HASH_MAP(p2puiClientDialog*, p2puiClientDialogHash);

class p2puiAboutDialog;

class p2puiFrame: public wxFrame
{    
    DECLARE_DYNAMIC_CLASS( p2puiFrame )
    DECLARE_EVENT_TABLE()
	
public:
    /// Constructors
    p2puiFrame();
    p2puiFrame(wxWindow* parent, wxWindowID id = -1, const wxString& caption = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX );

    ~p2puiFrame();

    bool Create( wxWindow* parent, wxWindowID id = -1, const wxString& caption = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxMINIMIZE_BOX|wxMAXIMIZE_BOX|wxCLOSE_BOX );

    /// Creates the controls and sizers
    void CreateControls();

    virtual cfg_t *GetCfg();

////@begin p2puiFrame event handler declarations

    /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME
    void OnCloseWindow( wxCloseEvent& event );

    /// wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED event handler for ID_NOTEBOOK
    void OnNotebookPageChanged( wxNotebookEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_NOTEBOOK
    void OnNotebookUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ACCOUNT_CONFIGURE
    void OnAccountConfigureClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CONNECT_BUTTON
    void OnConnectButtonClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_CONNECT_BUTTON
    void OnConnectButtonUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_CONNECT_TEXT
    void OnConnectTextSelected( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DISCONNECT
    void OnDisconnectClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_DISCONNECT
    void OnDisconnectUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CONNECT_RUN_BUTTON
    void OnConnectRunButtonClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_CONNECT_RUN_BUTTON
    void OnConnectRunButtonUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for ID_LOGIN_ACCEPT_LIST
    void OnLoginAcceptListSelected( wxListEvent& event );

    /// wxEVT_COMMAND_LIST_DELETE_ITEM event handler for ID_LOGIN_ACCEPT_LIST
    void OnLoginAcceptListDeleteItem( wxListEvent& event );

    /// wxEVT_COMMAND_LIST_ITEM_SELECTED event handler for ID_LOGIN_CONNECT_LIST
    void OnLoginConnectListSelected( wxListEvent& event );

    /// wxEVT_COMMAND_LIST_DELETE_ITEM event handler for ID_LOGIN_CONNECT_LIST
    void OnLoginConnectListDeleteItem( wxListEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_LOGIN_HOSTNAME
    void OnLoginHostnameUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_LOGIN_SAVE
    void OnLoginSaveClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_LOGIN_SAVE
    void OnLoginSaveUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_LOGIN_CLEAR
    void OnLoginClearClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_LOGIN_DELETE
    void OnLoginDeleteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_LOGIN_ACCEPT_LOCAL
    void OnLoginAcceptLocalClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_LOGIN_ACCEPT_ANY
    void OnLoginAcceptAnyClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_LOGIN_OPTIONS_SAVE
    void OnLoginOptionsSaveClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_LOGIN_OPTIONS_SAVE
    void OnLoginOptionsSaveUpdate( wxUpdateUIEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_APP_LABEL
    void OnAppLabelUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_APP_ICON
    void OnAppIconClick( wxCommandEvent& event );

    /// wxEVT_UPDATE_UI event handler for ID_APP_CMDLINE
    void OnAppCmdlineUpdate( wxUpdateUIEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_APP_BROWSE
    void OnAppBrowseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_APP_ICON_UPDATE
    void OnAppIconUpdateClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_APP_PORT
    void OnAppPortUpdated( wxCommandEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_APP_AUTORUN
    void OnAppAutorunClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_APP_SAVE
    void OnAppSaveClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_APP_RUN
    void OnAppRunClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_APP_NEW
    void OnAppNewClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_APP_DELETE
    void OnAppDeleteClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FILE_UPDATES
    void OnMenuFileUpdatesClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FILE_SHUTDOWN
    void OnMenuFileShutdownClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FILE_CLOSE
    void OnMenuFileCloseClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_FILE_HIDE
    void OnMenuFileHideClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_CONFIGURE_ACCOUNT
    void OnMenuConfigureAccountClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_HELP_CONTENTS
    void OnMenuHelpContentsClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_MENU_SELECTED event handler for ID_MENU_HELP_ABOUT
    void OnMenuHelpAboutClick( wxCommandEvent& event );

////@end p2puiFrame event handler declarations

////@begin p2puiFrame member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );
////@end p2puiFrame member function declarations

    /// Should we show tooltips?
    static bool ShowToolTips();

    int TryConnectPeer();
    int ConnectPeer();
    
    virtual void OnAppListenListClick( wxCommandEvent& event );
    virtual void OnAppCallListClick( wxCommandEvent& event );

    virtual void OnConnectedListClick( wxCommandEvent& event );
    virtual void OnConnectedListDClick( wxCommandEvent& event );
    virtual void OnConnectedListRightClick( wxCommandEvent& event );

    // refresh the list in the current notebook page
    void RefreshPage();

    void OnQuit(wxCommandEvent& event);

    virtual void OnAbout(wxCommandEvent &event);

    virtual void OnTimer(wxTimerEvent &event);

    virtual void OnSocket(wxSocketEvent& event);

    virtual void OnConnected();

    virtual p2puiCtlPeerConn* GetConn();

    static void OnCtlPeerRecv_s(p2puiCtlPeerConn *conn, p2p_ctl_peer_t *msg, 
				int incoming, void *arg);

    virtual void OnCtlPeerRecv(p2puiCtlPeerConn *conn, p2p_ctl_peer_t *msg, 
			       int incoming);

    virtual void AddConnectedClient(const char *hostname);

    // start connecting to a client, or search for matches
    virtual void ConnectClient(const char *hostname); 

    virtual void Shutdown();

    virtual void StatusMessage(const char *msg);

    virtual void SetAutoUpdate(int val);

    virtual void CheckForUpdates();

    virtual p2puiUpdateDialog *GetUpdateDialog();

    virtual wxString GetPeerStatusHtml();

    virtual void SetPeerStatus(p2p_ctl_peer_status_t *m=0);
	
    // return a search dialog for this string, or create and display a new one
    virtual p2puiSearchResultDialog* GetSearchResultDialog(const char *search_string, int create);

    virtual void DestroySearchResultDialog(p2puiSearchResultDialog *dlg);

    virtual void SetAppEditInfo(p2p_ctl_peer_app_info_t *info);
    virtual void UpdateAppIcon(const char *path=0);

    virtual p2puiHostListBox* GetConnectedListBox();

    virtual void ShowHelp(const char *page=0);

////@begin p2puiFrame member variables
    wxNotebook* m_notebook;
    p2puiAccountHtmlWindow* m_account_html;
    wxButton* m_connect_button;
    KeyComboBox* m_connect_text;
    p2puiHostListBox* m_connected_list;
    wxButton* m_disconnect_button;
    p2puiApplicationListPanel* m_connected_run_list;
    wxButton* m_connected_run_button;
    wxHtmlWindow* m_connect_html_help;
    wxListCtrl* m_login_accept_list;
    wxListCtrl* m_login_connect_list;
    wxStaticBox* m_accept_password_box;
    wxRadioButton* m_login_accept_radio;
    wxRadioButton* m_login_connect_radio;
    wxTextCtrl* m_login_hostname;
    wxTextCtrl* m_login_username;
    wxTextCtrl* m_login_password;
    wxTextCtrl* m_login_confirm;
    wxButton* m_login_save;
    wxCheckBox* m_login_accept_existing_check;
    wxCheckBox* m_login_accept_any_check;
    wxButton* m_login_options_save;
    wxHtmlWindow* m_login_html_help;
    p2puiApplicationListPanel* m_app_call_list;
    p2puiApplicationListPanel* m_app_listen_list;
    wxTextCtrl* m_app_label;
    wxBitmapButton* m_app_icon;
    wxTextCtrl* m_app_cmdline;
    wxComboBox* m_app_port;
    wxPanel* m_app_port_panel;
    wxCheckBox* m_app_allow_public;
    wxCheckBox* m_app_autorun;
    wxButton* m_app_save;
////@end p2puiFrame member variables

    wxSocketClient *m_ctl_sock;
    p2puiCtlPeerConn m_conn;
    wxTimer *m_timer;
    wxDateTime m_client_list_wait;

#if wxHAS_TASKBARICON
    wxTaskBarIcon *m_taskbaricon;
#endif

    int m_first_status;
    cfg_t *m_cfg;
    double m_connect_timeout;
    int m_shutdown;

    p2puiUpdateDialog *m_update;
    p2puiAccountDialog *m_account_dialog;
    p2puiSearchResultDialogArray m_search_dialogs; // hashed by search_string
    p2puiClientDialogHash m_client_dialog; // hashed by hostname
    int m_login_options_dirty;
    p2puiAboutDialog *m_about_dialog; // hashed by search_string

    wxString m_app_info_name; // name of app I'm editing, or "" for new
};

#endif
    // _P2P_UI_FRAME_H_

