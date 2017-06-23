#include <stdio.h>

#include "gkvid/glactorvideo.h"
#include "gkvid/glimpl.h"
#include "gkvid/copytask.h"
#include "gkvid/codecfactory.h"

//------------------------------------------------------------------

GLActorVideo::GLActorVideo() {
    m_tex_name = -1;
    m_bmp_width = 0;
    m_bmp_height = 0;
    m_tex_width = 0;
    m_tex_height = 0;
    m_render_task = 0;
    m_codec_task = 0;
    m_avi_queue = 0;
    m_priority = TaskScheduler::PRIORITY_HIGH;
    m_avi_queue = 0;
    m_img_vflip = 0;
}

GLActorVideo::~GLActorVideo() {
    PlayStop();

}

int
GLActorVideo::Init(GLPlayer *player, const char *name) {
    return GLActor::Init(player, name);
}

int
GLActorVideo::PlayStop() {
    int i;

    do {
	i = GLPlayerActor::PlayStop();

	REFCOUNT_RELEASE(m_render_task);
	REFCOUNT_RELEASE(m_codec_task);
	REFCOUNT_RELEASE(m_avi_queue);

	if( m_tex_name >= 0 ) {
	    assertb_gl();
	    glDeleteTextures(1, (GLuint*)&m_tex_name);
	    m_tex_name = -1;
	    assertb_gl();
	}
    } while(0);
    
    return i;
}

int
GLActorVideo::ConnectQueue(MsgQueue *avi_queue) {
    int i, err=-1;
    MsgQueue *codec_queue=0;
    TaskScheduler *sched=0;
    GLPlayer *player = 0;
    Val v;
    char buf[1024];

    do {
	PlayStop();
	m_avi_queue = avi_queue;

	if( !m_avi_queue ) {
	    err = 0;
	    break;
	}
	m_avi_queue->AddRef();

	/* make sure this is an image stream */
	i = m_avi_queue->GetVal("img_codec_id", v);
	assertb(i);

	player = GetPlayer();
	assertb(player);
	sched = player->GetScheduler();
	assertb(sched);

	/* find a codec for this stream */
	m_codec_task = TheCodecFactory.FindCodec(m_avi_queue, 0);
	assertb(m_codec_task);

	codec_queue = m_codec_task->GetQueue(1);
	if( !codec_queue ) {
	    codec_queue = new MsgQueue();
	    debug_leak_create_assertb(codec_queue);
	    codec_queue->Init(m_avi_queue->GetQueueMax());
	    i = m_codec_task->ConnectQueue(codec_queue, QUEUE_OUT, 1);
	}
	i = sched->AddTask(m_codec_task, m_priority);
	assertb(i>=0);
	AddSubtask(m_codec_task);
	
	snprintf(buf, sizeof(buf), "%s/VideoCodecTask", m_avi_queue->GetQueueName());
	m_codec_task->SetTaskName(buf);
	//debug(DEBUG_INFO, ("%s=%p\n", buf, m_codec_task));

	/* a task to collect decoded bitmaps and refresh the display */
	m_render_task = new GLActorDataTask();
	debug_leak_create_assertb(m_render_task);
	i = m_render_task->Init(this);
	assertb(i>=0);
	i = m_render_task->ConnectQueue(codec_queue, QUEUE_IN, 0);
	assertb(i>=0);
	AddSubtask(m_render_task);
	i = sched->AddTask(m_render_task, m_priority);
	assertb(i>=0);
	snprintf(buf, sizeof(buf), "%s/VideoDataTask", m_avi_queue->GetQueueName());
	m_render_task->SetTaskName(buf);

	snprintf(buf, sizeof(buf), "%s/VideoRenderTask", m_avi_queue->GetQueueName());
	m_render_task->SetTaskName(buf);

	/* make myself a video texture to render images */
	i = m_avi_queue->GetVal("img_width", v);
	assertb(i);
	m_bmp_width = v.val.i;	
	i = m_avi_queue->GetVal("img_height", v);
	assertb(i);
	m_bmp_height = v.val.i;

	// bitmaps can change size later sue to scaling for faster
	// decomression.  I use this as my default pixel size, then
	// scale my image later in Draw()
	SetPixelSize(m_bmp_width, m_bmp_height);
	
	SetVal("actor_video", (int)1);
	player->AddActor(this);
	
	err = 0;

    } while(0);

    REFCOUNT_RELEASE(codec_queue);
    REFCOUNT_RELEASE(sched);
    REFCOUNT_RELEASE(player);

    return err;
}

int
GLActorVideo::Draw() {
    int err=-1;
    MsgQueueMsg *msg = 0;
    size_t len_in, max_in;
    char *buf_in;
    Val val;

    PushTransform();

    do {
	if( m_draw_picking ) {
	    assertb_gl();

	    if( m_bmp_width > 0 && m_bmp_height > 0 ) {
		glScaled((double)m_w / m_bmp_width, (double)m_h / m_bmp_height, 0);
	    }

	    glColor3f(0,0,0);
	    glBegin(GL_QUADS); 
	    {
		GLfloat w = (GLfloat)m_bmp_width;
		GLfloat h = (GLfloat)m_bmp_height;

		glVertex3f(0, 0, 0);
		glVertex3f(w, 0, 0);
		glVertex3f(w, h, 0);
		glVertex3f(0, h, 0);
	    }
	    glEnd();
	    assertb_gl();
	}
	else {

	    // the adv601 codec can stop decoding at a lower scale to
	    // save CPU cycles.
	    if( m_codec_task ) {
		LayoutBox *box = GetLayoutBox();
		if( box ) {
		    m_codec_task->SetVal("img_scale_width", 
					(int)(box->m_inner_width));
		    m_codec_task->SetVal("img_scale_height", 
					(int)(box->m_inner_height));
		    //debug(DEBUG_INFO, 
		    //	  ("GLActorVideo::Draw: img_scale_width=%d img_scale_height=%d name=%s\n"
		    //   , (int)(box->m_inner_width)
		    //   , (int)(box->m_inner_height)
		    //   , GetName()));
		}
	    }
    
	    /* render a new image if there's one available */
	    if( m_render_task ) {
		msg = m_render_task->PopMsg();
	    }
	    if( msg ) {
		// debug
		Val avi_stream_idx;
		msg->GetVal("avi_stream_idx", avi_stream_idx);
		debug(DEBUG_INFO, 
		      ("GLActorVideo::Draw rendered frame=%d name=%s\n",
		       avi_stream_idx.val.i, GetName()));
		
		if( msg->GetVal("img_width", val) ) {
		    m_bmp_width = val.val.i;
		}
		if( msg->GetVal("img_height", val) ) {
		    m_bmp_height = val.val.i;
		}

		/* make my image if I don't have it already */
		if( !(m_tex_name >= 0 
		      && m_tex_width >= m_bmp_width 
		      && m_tex_height >= m_bmp_height) ) {
		
		    if( m_tex_name >= 0 ) {
			// delete old texture if not the right size
			glDeleteTextures(1, (GLuint*)&m_tex_name);
			m_tex_name = -1;
		    }

		    /* make video texture */
		    glPushAttrib(GL_TEXTURE_BIT);
		    glEnable(GL_TEXTURE_2D);
		    glGenTextures(1, (GLuint*)&m_tex_name);
		    glBindTexture(GL_TEXTURE_2D, m_tex_name);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		    assertb_gl();
		    for(m_tex_width=1; m_tex_width<m_bmp_width; m_tex_width<<=1);
		    for(m_tex_height=1; m_tex_height<m_bmp_height; m_tex_height<<=1);
		    char *ptr = (char*)calloc(1, m_tex_width * m_tex_height * 3);
		    debug_leak_create_assertb(ptr);
		    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 
				 m_tex_width, m_tex_height, 0, 
				 GL_RGB, GL_UNSIGNED_BYTE, ptr);
		    debug_leak_delete(ptr);
		    free(ptr);
		    ptr = 0;
		    assertb_gl();

		    debug(DEBUG_INFO, 
			  ("GLActorVideo::Draw glGenTextures m_bmp=%d,%d m_tex=%d,%d tex_name=%d name=%s\n"
			   ,m_bmp_width, m_bmp_height
			   ,m_tex_width, m_tex_height 
			   ,m_tex_name
			   ,GetName()
			   ));

		    glPopAttrib();
		    assertb_gl();
		}
		
		len_in = msg->LockBuf(&buf_in, &max_in);
		assertb(buf_in);
		
		glPushAttrib(GL_TEXTURE_BIT);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_tex_name);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
				m_bmp_width, 
				m_bmp_height, 
				GL_RGB, GL_UNSIGNED_BYTE, 
				buf_in);
		glPopAttrib();

		m_img_vflip = msg->GetVal("img_vflip", val) && val.val.i;

		msg->ReleaseBuf(buf_in);
		assertb_gl();
	    }

	    /* draw my image */
	    assertb_gl();
	    glPushAttrib(GL_TEXTURE_BIT);
	    glPushMatrix();

	    GLfloat w = (GLfloat)m_bmp_width;
	    GLfloat h = (GLfloat)m_bmp_height;
	    GLfloat s = w / m_tex_width;
	    GLfloat t = h / m_tex_height;

	    if( m_tex_name >= 0 ) {
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_tex_name);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

		if( m_img_vflip ) {
		    glTranslatef(0, h, 0);
		    glScalef(1, -1, 1);
		}
	    }

	    // scale up my internal bitmap to stretch to m_w, m_h
	    glScaled((double)m_w / m_bmp_width, (double)m_h / m_bmp_height, 0);

	    glColor3f(0,0,0);

	    glBegin(GL_QUADS); {
		glTexCoord2f(0, 0); glVertex3f(0, 0, 0);
		glTexCoord2f(s, 0); glVertex3f(w, 0, 0);
		glTexCoord2f(s, t); glVertex3f(w, h, 0);
		glTexCoord2f(0, t); glVertex3f(0, h, 0);
	    }
	    glEnd();

	    glPopMatrix();
	    glPopAttrib();

	    assertb_gl();
	}

	err = 0;
    } while(0);
    PopTransform();

    REFCOUNT_RELEASE(msg);

    return err;
}

int
GLActorVideo::OnMouse(int event, int keys, int x, int y, int extra) {
    if( !(keys & GLPlayer::MOUSE_LEFT) ) {
	return 0;
    }
    GLPlayer *player = GetPlayer();
    if( player ) {
	player->LayoutSetMainVideo(this);
    }
    REFCOUNT_RELEASE(player);
    return 0;
}

int 
GLActorVideo::SetPriority(int priority) {
    int i, err=-1;
    TaskScheduler *sched=0;
	
    do {
	m_priority = priority;
	if( m_avi_queue ) {
	    i = m_avi_queue->SetPriority(priority);
	    assertb(i>=0);
	}
	if( m_codec_task ) {
	    i = m_codec_task->SetPriority(priority);
	    assertb(i>=0);
	}
	if( m_render_task ) {
	    i = m_render_task->SetPriority(priority);
	    assertb(i>=0);
	}
	err = 0;
    } while(0);
    REFCOUNT_RELEASE(sched);
    return err;
}

int
GLActorVideo::Layout(LayoutBox *box, GLTime t0,  GLTime dt) {
    GLTime   t[2];
    GLVec    v[2];
    int      aw, ah;
    BoxUnit  scale;
    do {
	/* go from t0 to t0+dt */
	t[0] = t0 + dt;
	t[1] = t0 + dt; /* repeat end point to remain constant */

	/* scale */
	GetPixelSize(&aw, &ah);
	scale = ScaleRect((BoxUnit)aw, (BoxUnit)ah, box->m_inner_width, box->m_inner_height);
	v[0].Set3(scale, scale, 1);
	v[1] = v[0]; /* repeat the last ctl point to remain constant */
	GetScale()->SetTarget(t0, 2, t, v);

	/* translate */
	BoxUnit x = box->m_inner_left + (box->m_inner_width - scale * aw)/2;
	BoxUnit y = box->m_inner_bottom + (box->m_inner_height - scale * ah)/2;
	v[0].Set3(x, y, 0);
	v[1] = v[0];
	GetTranslate()->SetTarget(t0, 2, t, v);
	
    } while(0);
    return 0;
}
