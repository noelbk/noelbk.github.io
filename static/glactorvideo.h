/*------------------------------------------------------------------*/
/* 
   File: gk/src/gkvid/glactorvideo.h
   Author: Noel Burton-Krahn <noel@burton-krahn.coma>
   Created: Sep, 2005

   An Actor that draws a rendered OpenGL texture.  See the
   ConnectQueue method for how this actor constructs the tasks
   required to decode and render the raw video images from a
   AviPlayStream.
*/
#ifndef GLACTORVIDEO_H_INCLUDED
#define GLACTORVIDEO_H_INCLUDED

#include "gkvid/task.h"
#include "gkvid/glplayer.h"
#include "gkvid/glactordatatask.h"

class GLActorVideoTask;

class GLActorVideo : public GLPlayerActor {
public:
    GLActorVideo();
    virtual ~GLActorVideo();
    virtual int Init(GLPlayer *player, const char *name);

    virtual int ConnectQueue(MsgQueue *queue);

    virtual int PlayStop();

    virtual int Draw();
    virtual int OnMouse(int event, int keys, int x, int y, int extra);
    virtual int Layout(LayoutBox *box, GLTime t0, GLTime dt);

    virtual int SetPriority(int priority);

protected:
    int m_tex_name;
    int m_bmp_width;
    int m_bmp_height;
    int m_tex_width;
    int m_tex_height;
    int m_img_vflip;
    
    GLActorDataTask *m_render_task;
    Task *m_codec_task;
    MsgQueue *m_avi_queue;
    int m_priority;
};

#endif // GLACTORVIDEO_H_INCLUDED
