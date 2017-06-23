/*------------------------------------------------------------------*/
/* 
   File: gk/src/gkvid/GLPlayerCtlCtrl.h
   Author: Noel Burton-Krahn <noel@burton-krahn.coma>
   Created: Nov, 2005

   ActiveX wrapper around a GLPlayer object
*/
#pragma once

#include "gkvid/glplayer.h"

// CGLPlayerCtlCtrl : See GLPlayerCtlCtrl.cpp for implementation.
class CGLPlayerCtlCtrl : public COleControl
{
    DECLARE_DYNCREATE(CGLPlayerCtlCtrl);
    
public:
    CGLPlayerCtlCtrl();
    virtual ~CGLPlayerCtlCtrl();
    
    virtual void DoPropExchange(CPropExchange* pPX);
    virtual void OnResetState();
    virtual void OnClose(DWORD dwSaveOption);
    virtual BOOL IsOptimizedDraw();
    virtual DWORD GetControlFlags();

    afx_msg void AboutBox();
    afx_msg bool PlayStart();
    afx_msg bool PlayStop();
    afx_msg bool PlayListClear();
    afx_msg bool PlayListAppend(BSTR path);
    afx_msg long GetPlayListIndex();
    afx_msg long GetPlayListCount();
    afx_msg bool SetPlayListIndex(long index);
    afx_msg double GetPlayTime();
    afx_msg bool SetPlayTime(double secs);
    afx_msg long GetMainVideoIndex();
    afx_msg bool SetStatusText(BSTR text);
    afx_msg long GetPause();
    afx_msg long SetPause(long pause);
    afx_msg void DebugLog(BSTR text);
    afx_msg double GetCurrentDuration();
    afx_msg double GetPlayListIndexDuration(long index);

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnPaint();
    afx_msg void OnTimer(UINT timer_id);
    afx_msg void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
    afx_msg void OnLButtonDblClk(UINT, CPoint);
    afx_msg void OnLButtonDown(UINT, CPoint);
    afx_msg void OnLButtonUp(UINT, CPoint);
    afx_msg void OnMouseMove(UINT, CPoint);
    
protected:
    DECLARE_OLECREATE_EX(CGLPlayerCtlCtrl);    // Class factory and guid
    DECLARE_OLETYPELIB(CGLPlayerCtlCtrl);      // GetTypeInfo
    DECLARE_PROPPAGEIDS(CGLPlayerCtlCtrl);     // Property page IDs
    DECLARE_OLECTLTYPE(CGLPlayerCtlCtrl);      // Type name and misc status
    DECLARE_MESSAGE_MAP();
    DECLARE_DISPATCH_MAP();
    DECLARE_EVENT_MAP();

    // Dispatch and event IDs
public:
    enum {
    };

protected:
    virtual int Mouse(int event, int keys_in, int x, int y, int extra);
    virtual void Draw(HDC hdc);
    virtual void InitPlayer();
    virtual void ClosePlayer();
    static int PlayerCallback_s(GLPlayer::CallbackMsg *msg, void *farg);
    virtual int PlayerCallback(GLPlayer::CallbackMsg *msg);

    HGLRC    m_hRC;		//Rendering context
    HDC      m_hDC;		//Device Context
    GLPlayer *m_player;
    int m_timer_id;
    int m_timer;
};

