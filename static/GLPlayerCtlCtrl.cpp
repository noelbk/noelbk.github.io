// GLPlayerCtlCtrl.cpp : Implementation of the CGLPlayerCtlCtrl ActiveX Control class.

#include "GLPlayerCtlDef.h"
#include "stdafx.h"
#include "GLPlayerCtl.h"
#include "GLPlayerCtlCtrl.h"
#include "GLPlayerCtlPropPage.h"

#include "gkvid/glimpl.h"
#include "bklib/debug_stdio.h"

class GLPlayerLock {
public:
    GLPlayerLock(GLPlayer *player, HDC hdc, HGLRC rc);
    ~GLPlayerLock();
    int ok;

protected:
    static int m_refcount;
    HDC m_hdc;
    HGLRC m_hrc;
    GLPlayer *m_player; 
};

int GLPlayerLock::m_refcount = 0;

/* utility class to call wglMakeCurrent(dc, rc) */
GLPlayerLock::GLPlayerLock(GLPlayer *player, HDC hdc, HGLRC hrc) {
    ok = 0;
    do {
	//debug(DEBUG_INFO, ("GLPlayerLock hdc=%p hrc=%p count=%d\n", hdc, hrc, m_refcount));
	 
	m_player = player;
	m_hdc = hdc;
	m_hrc = hrc;

	if( !m_hdc && m_hrc ) {
	    break;
	}

	assertb(m_refcount>=0);
	m_refcount++;
	if( m_refcount > 1 ) {
	    ok = 1;
	    break;
	}


	//i = wglMakeCurrent(m_hdc, m_hrc);
	//assertb_syserr(i);

	assertb(m_player);

	ok = 1;
    } while(0);
}

GLPlayerLock::~GLPlayerLock() {
    do {
	//debug(DEBUG_INFO, ("GLPlayerUnlock hdc=%p rc=%p count=%d\n", m_hdc, m_hrc, m_refcount));
	if( !ok ) {
	    break;
	}
	ok = 0;

	assertb(m_refcount > 0);
	m_refcount--;
	if( m_refcount > 0 ) {
	    break;
	}

	assertb(m_hdc && m_hrc);

	//i = wglMakeCurrent(m_hdc, m_hrc);
	//assertb(i);

    } while(0);
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CGLPlayerCtlCtrl, COleControl);

// Message map
BEGIN_MESSAGE_MAP(CGLPlayerCtlCtrl, COleControl)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_PAINT()
    ON_WM_TIMER()
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONUP()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONDBLCLK()
    ON_WM_TIMER()

    ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
    END_MESSAGE_MAP()
    ;

// Dispatch map
BEGIN_DISPATCH_MAP(CGLPlayerCtlCtrl, COleControl)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "PlayListClear", DISPID_PlayListClear, PlayListClear, VT_BOOL, VTS_NONE)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "PlayListAppend", DISPID_PlayListAppend, PlayListAppend, VT_BOOL, VTS_BSTR)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "GetPlayListCount", DISPID_GetPlayListCount, GetPlayListCount, VT_I4, VTS_NONE)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "GetPlayListIndex", DISPID_GetPlayListIndex, GetPlayListIndex, VT_I4, VTS_NONE)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "SetPlayListIndex", DISPID_SetPlayListIndex, SetPlayListIndex, VT_BOOL, VTS_I4)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "GetPlayTime", DISPID_GetPlayTime, GetPlayTime, VT_R8, VTS_NONE)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "SetPlayTime", DISPID_SetPlayTime, SetPlayTime, VT_BOOL, VTS_R8)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "PlayStart", DISPID_PlayStart, PlayStart, VT_BOOL, VTS_NONE)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "PlayStop", DISPID_PlayStop, PlayStop, VT_BOOL, VTS_NONE)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "GetMainVideoIndex", DISPID_GetMainVideoIndex, GetMainVideoIndex, VT_I4, VTS_NONE)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "SetStatusText", DISPID_SetStatusText, SetStatusText, VT_BOOL, VTS_BSTR)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "GetPause", DISPID_GetPause, GetPause, VT_I4, VTS_NONE)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "SetPause", DISPID_SetPause, SetPause, VT_I4, VTS_I4)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "DebugLog", DISPID_DebugLog, DebugLog, VT_EMPTY, VTS_BSTR)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "GetCurrentDuration", DISPID_GetCurrentDuration, GetCurrentDuration, VT_R8, VTS_NONE)
    DISP_FUNCTION_ID(CGLPlayerCtlCtrl, "GetPlayListIndexDuration", DISPID_GetPlayListIndexDuration, GetPlayListIndexDuration, VT_R8, VTS_I4)

    END_DISPATCH_MAP()
    ;

// Event map
BEGIN_EVENT_MAP(CGLPlayerCtlCtrl, COleControl)
    END_EVENT_MAP()

    ;


// Property pages
// Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CGLPlayerCtlCtrl, 1)
    PROPPAGEID(CGLPlayerCtlPropPage::guid)
    END_PROPPAGEIDS(CGLPlayerCtlCtrl)
    ;

// Initialize class factory and guid
IMPLEMENT_OLECREATE_EX(CGLPlayerCtlCtrl, "GLPLAYERCTL.GLPlayerCtlCtrl.1",
		       0x5f5b20fc, 0xfa41, 0x4132, 0x88, 0xf2, 0x25, 0x74, 0xa5, 0xc5, 0x49, 0x3e);


// Type library ID and version
IMPLEMENT_OLETYPELIB(CGLPlayerCtlCtrl, _tlid, _wVerMajor, _wVerMinor);

// Interface IDs
const IID BASED_CODE IID_DGLPlayerCtl =
    { 0xCBAC340F, 0x3D65, 0x4666, { 0xB7, 0x22, 0xBA, 0x27, 0x29, 0x9, 0x5C, 0x19 } };
const IID BASED_CODE IID_DGLPlayerCtlEvents =
    { 0xF1294EF7, 0x2261, 0x4B72, { 0x85, 0x31, 0xAC, 0x7D, 0x8B, 0x63, 0xB8, 0xEF } };

// Control type information
static const DWORD BASED_CODE _dwGLPlayerCtlOleMisc =
OLEMISC_ACTIVATEWHENVISIBLE |
OLEMISC_SETCLIENTSITEFIRST |
OLEMISC_INSIDEOUT |
OLEMISC_CANTLINKINSIDE |
OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CGLPlayerCtlCtrl, IDS_GLPLAYERCTL, _dwGLPlayerCtlOleMisc)
    ;

// CGLPlayerCtlCtrl::CGLPlayerCtlCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CGLPlayerCtlCtrl
BOOL
CGLPlayerCtlCtrl::CGLPlayerCtlCtrlFactory::UpdateRegistry(BOOL bRegister)
{
    // TODO: Verify that your control follows apartment-model threading rules.
    // Refer to MFC TechNote 64 for more information.
    // If your control does not conform to the apartment-model rules, then
    // you must modify the code below, changing the 6th parameter from
    // afxRegApartmentThreading to 0.
    if (bRegister)
	return AfxOleRegisterControlClass(
					  AfxGetInstanceHandle(),
					  m_clsid,
					  m_lpszProgID,
					  IDS_GLPLAYERCTL,
					  IDB_GLPLAYERCTL,
					  0, // afxRegApartmentThreading,
					  _dwGLPlayerCtlOleMisc,
					  _tlid,
					  _wVerMajor,
					  _wVerMinor);
    else
	return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

// CGLPlayerCtlCtrl::CGLPlayerCtlCtrl - Constructor
CGLPlayerCtlCtrl::CGLPlayerCtlCtrl()
{
    InitializeIIDs(&IID_DGLPlayerCtl, &IID_DGLPlayerCtlEvents);

#if 0
    debug_init(DEBUG_INFO, debug_func_log, 
	       debug_func_log_init("c:/tmp/glplayerctl/glplayerctl", 3, 10000000, stderr));
    debug(DEBUG_INFO, ("CGLPlayerCtlCtrl::CGLPlayerCtlCtrl\n"));
#else     
    debug_init(DEBUG_WARN, 0, 0);
#endif

    m_player = 0;
    m_hDC = 0;
    m_hRC = 0;
}


// CGLPlayerCtlCtrl::~CGLPlayerCtlCtrl - Destructor
CGLPlayerCtlCtrl::~CGLPlayerCtlCtrl()
{
    //debug(DEBUG_INFO, ("CGLPlayerCtlCtrl::~CGLPlayerCtlCtrl\n"));
    ClosePlayer();
}

// CGLPlayerCtlCtrl message handlers
void
CGLPlayerCtlCtrl::ClosePlayer() {
    //debug(DEBUG_INFO, ("CGLPlayerCtlCtrl::ClosePlayer\n"));

    if( m_player ) {
	GLPlayerLock lock(m_player, m_hDC, m_hRC);
	m_player->Close();
	REFCOUNT_RELEASE(m_player);
    }

    if( m_hRC ) {
	wglDeleteContext(m_hRC);
	m_hRC = 0;
    }

    debug_leak_dump();
    debug_leak_fini();
}


// CGLPlayerCtlCtrl::DoPropExchange - Persistence support
void CGLPlayerCtlCtrl::DoPropExchange(CPropExchange* pPX)
{
    ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
    COleControl::DoPropExchange(pPX);
    // TODO: Call PX_ functions for each persistent custom property.
}

// CGLPlayerCtlCtrl::OnResetState - Reset control to default state
void CGLPlayerCtlCtrl::OnResetState()
{
    COleControl::OnResetState();  // Resets defaults found in DoPropExchange

    // TODO: Reset any other control state here.
}

// CGLPlayerCtlCtrl::AboutBox - Display an "About" box to the user
void CGLPlayerCtlCtrl::AboutBox()
{
    CDialog dlgAbout(IDD_ABOUTBOX_GLPLAYERCTL);
    dlgAbout.DoModal();
}

void
CGLPlayerCtlCtrl::OnClose(DWORD dwSaveOption) {

    //debug(DEBUG_INFO, ("CGLPlayerCtlCtrl::OnClose\n"));

    ClosePlayer();
    COleControl::OnClose(dwSaveOption);
}

BOOL
CGLPlayerCtlCtrl::IsOptimizedDraw() {
    //debug(DEBUG_INFO, ("CGLPlayerCtlCtrl::IsOptimizedDraw\n"));
    return TRUE;
}

void
CGLPlayerCtlCtrl::OnDestroy() {
    m_hDC = 0;

    //debug(DEBUG_INFO, ("CGLPlayerCtlCtrl::OnDestroy\n"));
    ClosePlayer();
}


int
CGLPlayerCtlCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    int i, err=-1;

    do {
	debug(DEBUG_INFO, 
	      ("CGLPlayerCtlCtrl::OnCreate m_hWnd=%p\n", m_hWnd));

	if( m_hRC ) {
	    if( m_hDC ) {
		wglMakeCurrent(NULL, NULL);
	    }
	    wglDeleteContext(m_hRC);
	    m_hRC = 0;
	}
	if( m_hDC ) {
	    ::ReleaseDC(m_hWnd, m_hDC);
	    m_hDC = 0;
	}

	lpCreateStruct->style |= (WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CS_OWNDC);

	i = COleControl::OnCreate(lpCreateStruct);
	assertb(i>=0);

	//get device context 
	m_hDC = ::GetDC(m_hWnd);
	assertb(m_hDC);

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize        = sizeof(pfd);
	pfd.nVersion     = 1;
	pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType   = PFD_TYPE_RGBA;
	pfd.cDepthBits   = 32;
	pfd.cColorBits   = 32;

	int pixelFormat;
	pixelFormat = ChoosePixelFormat(m_hDC, &pfd);
	i = SetPixelFormat(m_hDC, pixelFormat, &pfd);
	assertb(i);

	DescribePixelFormat(m_hDC, pixelFormat,
			    sizeof(pfd), &pfd);

	//if(pfd.dwFlags & PFD_NEED_PALETTE)
	//SetupPalette();

	m_hRC = wglCreateContext(m_hDC);
	assertb(m_hRC);

	wglMakeCurrent(m_hDC, m_hRC);

	InitPlayer();

	//wglMakeCurrent(m_hDC, NULL);

	Refresh();
    } while(0);

    return 0;
}

void
CGLPlayerCtlCtrl::OnPaint() {
    CPaintDC dc(this); // device context for painting
    Draw(dc.m_hDC);
}

void
CGLPlayerCtlCtrl::OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid) {
    Draw(pdc->m_hDC);
}

DWORD
CGLPlayerCtlCtrl::GetControlFlags() {
    DWORD flags = COleControl::GetControlFlags();
    flags &= ~ windowlessActivate;
    return flags;
}

void
CGLPlayerCtlCtrl::Draw(HDC hdc) {
    GLPlayerLock lock(m_player, hdc, m_hRC);

    do {
	int w, h, pw, ph;
	GetControlSize(&w, &h);

	glViewport(0, 0, w, h);
	glClear(GL_COLOR_BUFFER_BIT);

	assertb(lock.ok);
	assertb(m_player);

	m_player->GetSize(&pw, &ph);
	if( pw != w || ph != h ) {
	    m_player->SetSize(w, h);
	    m_player->Layout(0);
	}

	m_player->Draw();

	SwapBuffers(hdc);

    } while(0);
}

int
CGLPlayerCtlCtrl::PlayerCallback_s(GLPlayer::CallbackMsg *msg, void *farg) {
    return ((CGLPlayerCtlCtrl*)farg)->PlayerCallback(msg);
}

int
CGLPlayerCtlCtrl::PlayerCallback(GLPlayer::CallbackMsg *msg) {
    switch(msg->type) {
    case GLPlayer::CALLBACK_REDRAW:
	//debug(DEBUG_INFO, ("PlayerCallback CALLBACK_REDRAW\n"));
	InvalidateRect(0);
	//RedrawWindow(hWnd, 0, 0, RDW_INVALIDATE);
	break;

    case GLPlayer::CALLBACK_CAPTURE_MOUSE:
	SetCapture();
	break;

    case GLPlayer::CALLBACK_RELEASE_MOUSE:
	ReleaseCapture();
	break;
    }
    return 0;
}

bool
CGLPlayerCtlCtrl::PlayStop() {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->PlayStop();
    } while(0);
    KillTimer(m_timer);
    return TRUE;
}

bool
CGLPlayerCtlCtrl::PlayStart() {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);

    do {
	assertb(lock.ok);
#if 0
	char *play_list[] = {
	    ""

	    ,"d:/video/test.smi"
	    ,"d:/video/test-noel.avi"
	    ,"d:/video/test-noel.avi"
	    ,""

	    ,"d:/video/test-kids.avi"
	    ,"d:/video/test-kids.avi"
	    ,""

	    ,"d:/video/test.smi"
	    ,"d:/video/test-jeeva.avi"
	    ,"d:/video/test-jeeva.avi"
	    ,"d:/video/test-jeeva.avi"
	    ,"d:/video/test-jeeva.avi"
	    ,""

	    ,0
	};

	assertb(m_player);
	m_player->PlayListClear();
	for(char **pp = play_list; *pp; pp++) {
	    m_player->PlayListAppend(*pp);
	}
#endif

	m_player->PlayStart();

    } while(0);

    m_timer_id = 29387;
    m_timer = SetTimer(m_timer_id, 1000/4 , 0);
    return TRUE;
}

void
CGLPlayerCtlCtrl::InitPlayer() {
    //debug(DEBUG_INFO, ("CGLPlayerCtlCtrl::InitPlayer m_player=%p\n", m_player));
    int err = -1;
    do {
	if( m_player ) {
	    REFCOUNT_RELEASE(m_player);
	    m_player = 0;
	}
	 
	m_player = new GLPlayer();
	debug_leak_create_assertb(m_player);
	m_player->Init();
	m_player->SetCallback(PlayerCallback_s, this);
    } while(0);
}

int
CGLPlayerCtlCtrl::Mouse(int event, int keys_in, int x, int y, int extra) {
    int ret=0;
    int keys = 0;
    GLPlayerLock lock(m_player, m_hDC, m_hRC);

    do {
	assertb(lock.ok);
	assertb(m_player);

	if( keys_in & MK_CONTROL )   { keys |= GLPlayer::MOUSE_CONTROL; }
	if( keys_in & MK_LBUTTON )   { keys |= GLPlayer::MOUSE_LEFT; }
	if( keys_in & MK_MBUTTON )   { keys |= GLPlayer::MOUSE_MIDDLE; }
	if( keys_in & MK_RBUTTON )   { keys |= GLPlayer::MOUSE_RIGHT; }
	if( keys_in & MK_SHIFT )     { keys |= GLPlayer::MOUSE_SHIFT; }

	ret = m_player->OnMouse(event, keys, x, y, extra);
    } while(0);

    return ret;
}

void
CGLPlayerCtlCtrl::OnLButtonDown(UINT keys, CPoint p) {
    Mouse(GLPlayer::MOUSE_DOWN | GLPlayer::MOUSE_LEFT, keys, p.x, p.y, 0);
}

void
CGLPlayerCtlCtrl::OnLButtonUp(UINT keys, CPoint p) {
    Mouse(GLPlayer::MOUSE_UP | GLPlayer::MOUSE_LEFT, keys, p.x, p.y, 0);
}

void
CGLPlayerCtlCtrl::OnLButtonDblClk(UINT keys, CPoint p) {
    Mouse(GLPlayer::MOUSE_DOUBLE | GLPlayer::MOUSE_LEFT, keys, p.x, p.y, 0);
}

void
CGLPlayerCtlCtrl::OnMouseMove(UINT keys, CPoint p) {
    Mouse(GLPlayer::MOUSE_MOVE, keys, p.x, p.y, 0);
}

void
CGLPlayerCtlCtrl::OnTimer(UINT_PTR nIDEvent) {
    //debug(DEBUG_INFO, ("CGLPlayerCtlCtrl::OnTimer\n"));
    InvalidateRect(0);
    //RedrawWindow(hWnd, 0, 0, RDW_INVALIDATE);
}

bool
CGLPlayerCtlCtrl::PlayListClear() {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);

    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->PlayListClear();
    } while(0);

    return TRUE;
}

bool
CGLPlayerCtlCtrl::PlayListAppend(BSTR path) {
    // !@#^%ing BSTRs!  Could anything be more !@#$'ed up?  Magically,
    // this appears to be an ansi 8-bit char*, not a 16-bit wide
    // character string.


    GLPlayerLock lock(m_player, m_hDC, m_hRC);

    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->PlayListAppend((char*)path);

    } while(0);

    return TRUE;
}

long
CGLPlayerCtlCtrl::GetPlayListCount() {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    GLPlayer::PlayState ps;
    
    ps.playlist_count = 0;
    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->GetPlayState(&ps);
    } while(0);
    return ps.playlist_count;
}

long
CGLPlayerCtlCtrl::GetPlayListIndex() {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    GLPlayer::PlayState ps;
    
    ps.playlist_index = 0;
    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->GetPlayState(&ps);
    } while(0);
    return ps.playlist_index;
}

double
CGLPlayerCtlCtrl::GetPlayListIndexDuration(long idx) {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    double d = 0;
    do {
	assertb(lock.ok);
	assertb(m_player);

	d = m_player->GetPlayListIndexDuration(idx);
    } while(0);
    return d;
}

double
CGLPlayerCtlCtrl::GetCurrentDuration() {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    GLPlayer::PlayState ps;
    
    ps.playlist_index = 0;
    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->GetPlayState(&ps);
    } while(0);
    return ps.time_duration;
}

long
CGLPlayerCtlCtrl::GetMainVideoIndex() {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    GLPlayer::PlayState ps;
    
    ps.playlist_index = 0;
    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->GetPlayState(&ps);
    } while(0);
    return ps.main_video_index;
}

bool
CGLPlayerCtlCtrl::SetPlayListIndex(long idx) {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->SetPlayListIndex(idx);
    } while(0);
    return TRUE;
}

double
CGLPlayerCtlCtrl::GetPlayTime() {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    GLPlayer::PlayState ps;
    ps.play_time = 0;
    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->GetPlayState(&ps);
    } while(0);
    return ps.play_time;
}

bool
CGLPlayerCtlCtrl::SetPlayTime(double t) {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->SetPlayTime(t);
    } while(0);
    return TRUE;
}


bool
CGLPlayerCtlCtrl::SetStatusText(BSTR text) {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->SetStatusText((char*)text);
    } while(0);
    return TRUE;
}

long
CGLPlayerCtlCtrl::GetPause() {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    GLPlayer::PlayState ps;
    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->GetPlayState(&ps);
    } while(0);
    return ps.play_paused;
}

long
CGLPlayerCtlCtrl::SetPause(long pause) {
    GLPlayerLock lock(m_player, m_hDC, m_hRC);
    do {
	assertb(lock.ok);
	assertb(m_player);

	m_player->PlayPause(pause);
    } while(0);
    return 0;
}


void
CGLPlayerCtlCtrl::DebugLog(BSTR msg) {
    debug(DEBUG_WARN, ("DebugLog: %s\n", (char*)msg));
}
