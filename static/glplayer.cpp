#include "gkvid/glimpl.h"

#include "gkvid/glplayer.h"

#include "gkvid/aviplaytask.h"
#include "gkvid/avienum.h"
#include "gkvid/smiplaytask.h"

#include "gkvid/glactorvideo.h"
#include "gkvid/glactoraudio.h"
#include "gkvid/glactorplayslider.h"
#include "gkvid/glactorplaypause.h"
#include "gkvid/glactorsubtitle.h"
#include "gkvid/glactorplayspeed.h"
#include "gkvid/glactoraudiomute.h"

#include "bklib/defutil.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

struct PlayListGroup {
    array_t paths;
};


static void
PlayListGroup_array_free(void *elt, void *farg) {
    PlayListGroup *play_group = (PlayListGroup*)elt;
    array_clear(&play_group->paths);
}

static void
PlayTask_array_free(void *elt, void *farg) {
    PlayTask *play_task = *(PlayTask **)elt;
    GLPlayer *player = (GLPlayer *)farg;
    TaskScheduler *sched = 0;

    do {
	assertb(play_task);
	play_task->PlayStop();
	assertb(player);
	sched = player->GetScheduler();
	assertb(sched);
	sched->RemoveTask(play_task);
    } while(0);
    REFCOUNT_RELEASE(play_task);
    REFCOUNT_RELEASE(sched);
}

GLPlayer::GLPlayer() {
    do {
	m_play_thread = 0;
	m_cb_func = 0;
	m_cb_farg = 0;
	m_video_name_idx = 0;
	m_audio_name_idx = 0;
	m_sched = 0;

	m_mouse_capture = 0;
	m_mouse_capture_dummy = 0;

	m_width = 0;
	m_height = 0;

	m_font = 0;

	array_init(&m_play_tasks, sizeof(PlayTask*), 0);
	array_set_free(&m_play_tasks, PlayTask_array_free, this);
	
	array_init(&m_playlist, sizeof(PlayListGroup), 0);
	array_set_free(&m_playlist, PlayListGroup_array_free, this);
	m_playlist_idx = 0;

	m_main_video_idx = -1;

	m_play_paused = 0;
	m_play_speed = 1;

	m_audio_mute = 0;
	m_audio_volume = 1;

	m_play_state_cache_ok = 0;
	
	m_subvideo_priority = 0;

	m_font = 0;
    } while(0);
}

int
GLPlayer::Init() {
    int i, err=-1;
    do {
	i = GLWorld::Init();
	assertb(i>=0);
	    
	m_font = new GLFont();
	debug_leak_create_assertb(m_font);
	i = m_font->Init();
	assertb(i>=0);

	CreateControls();
	
	PlayStop();

	err = 0;
    } while(0);
    return err;
}

GLPlayer::~GLPlayer() {

    REFCOUNT_RELEASE(m_mouse_capture_dummy);

    if( m_font ) {
	debug_leak_delete(m_font);
	delete(m_font);
	m_font = 0;
    }
	
    Close();
}

int
GLPlayer::Close() {
    
    debug(DEBUG_INFO, ("GLPlayer::Close\n"));
    
    // kill my thread
    if( m_play_thread ) {
	void *v;
	m_play_thread_active = 0;
	thread_join(m_play_thread, &v);
	m_play_thread = 0;
    }

    /* don't threadunlock.  My thread is dead */

    PlayStop();
    
    DeleteControls();

    return 0;
}

int
GLPlayer::PlayStop() {
    GLWORLDTHREADLOCK(-1);

    int i, err=-1;

    do {
	for(i=GetActorCount()-1; i>=0; i--) {
	    GLPlayerActor *actor = (GLPlayerActor*)GetActor(i);
	    actor->PlayStop();
	    //actor->SetLayoutBox(0);
	    REFCOUNT_RELEASE(actor);
	}

	array_clear(&m_play_tasks);

	m_video_name_idx=0;
	m_audio_name_idx=0;
	m_play_state_cache_ok = 0;
	
	//Callback(CALLBACK_REDRAW);
	
	err = 0;
    } while(0);
    return err;
}

int
GLPlayer::PlayStart() {
    GLWORLDTHREADLOCK(-1);

    do {
	if( !m_play_thread ) {
	    m_play_thread = thread_new(PlayThread_s, this);
	    assertb(m_play_thread);
	    //thread_set_priority(m_play_thread, THREAD_PRIORITY_TIME_CRITICAL);
	}
	m_play_thread_active = 1;
    
	m_sched->SetVal("play_paused", (int)0);

	SetPlayListIndex(0);
    } while(0);

    return 0;
}


void*
GLPlayer::PlayThread_s(void *arg) {
    return ((GLPlayer*)arg)->PlayThread();
}

void*
GLPlayer::PlayThread() {
    int active;
    mstime_t play_time;

    //debug(DEBUG_INFO, ("GLPlayer::PlayThread\n"));

    while(1) {
	if( ThreadLock() < 0 ) continue;

	active = m_play_thread_active;
	if( active ) {
	    mstime_t t = mstime();
	    Val play_time_start, play_paused;
	    
	    m_sched->SetVal("time", t);
	    if( (m_sched->GetVal("play_paused", play_paused) 
		&& play_paused.val.i)
		|| m_play_speed == 0 ) {
		// don't update play_time
	    }
	    else {
		if( !m_sched->GetVal("play_time_start", play_time_start) ) {
		    play_time_start.val.d = t;
		    m_sched->SetVal("play_time_start", t);
		}
		m_sched->SetVal("play_speed", m_play_speed);
		play_time = (t - play_time_start.val.d) * m_play_speed;
		m_sched->SetVal("play_time", play_time);
	    }
	    
	    // HANDLE hthread = GetCurrentThread();
	    // 	    HANDLE hprocess = GetCurrentProcess();
	    // 	    debug(DEBUG_INFO, 
	    // 		  ("GLPlayer::PlayThread poll begin play_time=%lf thread_priority=%d priority_class=%d\n", 
	    // 		   play_time, GetThreadPriority(hthread), GetPriorityClass(hprocess)));
	    
	    m_sched->Poll();

	    // debug(DEBUG_INFO, ("GLPlayer::PlayThread poll done dt=%lf\n", mstime()-t));

	    // force PlatListRotate
	    PlayState ps;
	    GetPlayState(&ps);
	    if( ps.play_eof ) {
		Callback(CALLBACK_REDRAW);
	    }

	}
	ThreadUnlock();
	if( !active ) break;
    }
    return 0;
}

int
GLPlayer::Draw() {
    GLWORLDTHREADLOCK(-1);
    
    int err=-1;
    
    do {
	PlayListRotate();
	m_play_state_cache_ok = 0;
	GetPlayState(&m_play_state_cache);
	m_play_state_cache_ok = 1;

	GLWorld::Draw();

	err = 0;
    } while(0);

    return err;
}

int
GLPlayer::AddPlayTask(PlayTask *play_task) {
    GLWORLDTHREADLOCK(-1);

    *(PlayTask**)array_add(&m_play_tasks, 1) = play_task;
    play_task->AddRef();
    return 0;
}

int
GLPlayer::RemovePlayTask(PlayTask *play_task) {
    GLWORLDTHREADLOCK(-1);

    int i, found=0;
    for(i=array_count(&m_play_tasks)-1; i>=0; i--) {
	PlayTask *task = *(PlayTask**)array_get(&m_play_tasks, i);
	if( task == play_task ) {
	    array_remove_idx(&m_play_tasks, i, 1);
	    found++;
	}
    }
    return found;
}

int
GLPlayer::AddSmi(const char *smi_path) {
    GLWORLDTHREADLOCK(-1);

    SmiPlayTask *smi_task=0;
    MsgQueue *queue=0;
    int i, err=-1;
    GLPlayerActor *actor = 0;
    
    do {
	smi_task = new SmiPlayTask("subtitle");
	debug_leak_create_assertb(smi_task);
	i = smi_task->Init();
	assertb(i>=0);
	i = smi_task->Open(smi_path);
	assertb(i>=0);
	m_sched->AddTask(smi_task, TaskScheduler::PRIORITY_HIGH);
	
	AddPlayTask(smi_task);

	queue = smi_task->GetQueue(0);
	if( !queue ) {
	    queue = new MsgQueue;
	    debug_leak_create_assertb(queue);
	    queue->Init(1);
	    i = smi_task->ConnectQueue(queue, QUEUE_IN, 0);

	    assertb(i>=0);
	}
	
	actor = (GLPlayerActor*)FindActor("subtitle");
	if( !actor ) {
	    actor = new GLActorSubtitle();
	    debug_leak_create_assertb(actor);
	    actor->Init(this, "subtitle");
	    AddActor(actor);
	}
	assertb(actor);
	i = actor->ConnectQueue(queue);
	assertb(i>=0);

	err = 0;
    } while(0);

    REFCOUNT_RELEASE(queue);
    REFCOUNT_RELEASE(actor);
    REFCOUNT_RELEASE(smi_task);

    return err;
}

int
GLPlayer::AddAvi(const char *avi_path) {
    GLWORLDTHREADLOCK(-1);

    int i, err=-1;
    AviPlayTask *avi_task=0;
    GLPlayerActor *actor=0;
    Val codec_id;
    MsgQueue *queue=0;
    int queue_idx;
    TaskQueueDir dir;
    char name[1024];

    do {
	avi_task = new AviPlayTask(avi_path);
	debug_leak_create_assertb(avi_task);
	i = avi_task->Init();
	assertb(i>=0);
	i = avi_task->Open(avi_path);
	assertb(i>=0);
	m_sched->AddTask(avi_task, TaskScheduler::PRIORITY_HIGH);
	AddPlayTask(avi_task);

	for(queue_idx=0; queue_idx<avi_task->GetQueueCount(); queue_idx++) {
	    queue = avi_task->GetQueue(queue_idx, &dir);
	    if( dir != QUEUE_OUT ) {
		continue;
	    }
	    
	    if( queue->GetVal("img_codec_id", codec_id) ) {
		m_video_name_idx++;
		snprintf(name, sizeof(name), "video_%02d", m_video_name_idx);
		actor = (GLPlayerActor*)FindActor(name);
		if( !actor ) {
		    actor = new GLActorVideo();
		    debug_leak_create_assertb(actor);
		    actor->Init(this, name);
		}
		actor->ConnectQueue(queue);
	    }
	    if( queue->GetVal("audio_codec_id", codec_id) ) {
		m_audio_name_idx++;
		if( m_audio_name_idx == 1 ) {
		    snprintf(name, sizeof(name), "audio_%02d", m_audio_name_idx);
		    actor = (GLPlayerActor*)FindActor(name);
		    if( !actor ) {
			actor = new GLActorAudio();
			debug_leak_create_assertb(actor);
			actor->Init(this, name);
		    }
		    actor->ConnectQueue(queue);
		}
	    }
	    
	    REFCOUNT_RELEASE(actor);
	    REFCOUNT_RELEASE(queue);
	}
	
	err = 0;
    } while(0);

    REFCOUNT_RELEASE(avi_task);

    return err;
}

int
GLPlayer::PlayPause(int pause) {
    GLWORLDTHREADLOCK(-1);

    //debug(DEBUG_INFO, ("GLPlayer::PlayPause pause=%d\n", pause));

    m_play_paused = pause;
    m_sched->SetVal("play_paused", pause);

    if( !pause ) {
	Val play_time;
	if( !m_sched->GetVal("play_time", play_time) ) {
	    play_time.val.d = 0;
	}
	SetPlayTime(play_time.val.d);
    }
    UpdateAudio();

    return 0;
}

int
GLPlayer::SetAudioMute(int mute) {
    GLWORLDTHREADLOCK(-1);

    m_audio_mute = mute;
    m_sched->SetVal("audio_mute", m_audio_mute);
    return UpdateAudio();
}

int
GLPlayer::SetAudioVolume(double audio_volume) {
    GLWORLDTHREADLOCK(-1);

    m_audio_volume = audio_volume;
    m_sched->SetVal("audio_volume", m_audio_volume);
    SetAudioMute(0);
    return UpdateAudio();
}

int
GLPlayer::UpdateAudio() {
    GLWORLDTHREADLOCK(-1);

    int actor_idx;
    Val val;
    double volume = 0;

    volume = m_audio_volume;
    if( m_audio_mute || m_play_speed != 1 || m_play_paused ) {
	volume = 0;
    }

    for(actor_idx=0; actor_idx<GetActorCount(); actor_idx++) {
	GLActor *actor = GetActor(actor_idx);
	if( actor->GetVal("actor_audio", val) && val.val.i ) {
	    GLActorAudio *audio_actor = (GLActorAudio *)actor;
	    audio_actor->SetAudio(volume);
	}
	REFCOUNT_RELEASE(actor);
    }

    return 0;
}

int
GLPlayer::SetPlaySpeed(double play_speed) {
    GLWORLDTHREADLOCK(-1);

    PlayState ps;
    GetPlayState(&ps);

    if( ABS(play_speed) > GLPLAYER_PLAY_SPEED_MAX ) {
	play_speed = GLPLAYER_PLAY_SPEED_MAX * SIGN(play_speed);
    }
    if( ABS(play_speed) <= 0 ) {
	play_speed = 0;
    }

    m_play_speed = play_speed;
    SetPlayTime(ps.play_time);

    // turn off audio if play_speed != 1
    UpdateAudio();

    return 0;
}


int
GLPlayer::SetPlayTime(TaskTime play_time) {
    GLWORLDTHREADLOCK(-1);

    int idx;

    do {
	if( play_time < 0 ) {
	    play_time = 0;
	}
	m_sched->SetVal("play_time", play_time);
 	m_sched->SetVal("play_time_start", 
			m_play_speed == 0 ? play_time : mstime() - play_time / m_play_speed);

	debug(DEBUG_INFO, ("GLPlayer::SetPlayTime play_time=%lf\n", play_time));

	for(idx=array_count(&m_play_tasks)-1; idx>=0; idx--) {
	    PlayTask *play_task = *(PlayTask**)array_get(&m_play_tasks, idx);
	    play_task->PlayReset();
	}
    } while(0);

    return 0;
}

int
GLPlayer::GetPlayState(GLPlayer::PlayState *play_state) {
    GLWORLDTHREADLOCK(-1);

    int idx;
    Val val;

    if( m_play_state_cache_ok ) {
	*play_state = m_play_state_cache;
	return 0;
    }

    memset(play_state, 0, sizeof(*play_state));
    
    play_state->subvideo_priority = m_subvideo_priority;

    play_state->main_video_index = -1;
    GLActor *actor = GetActor(m_main_video_idx);
    if( actor ) {
	if( actor->GetVal("main_video_idx", val) ) {
	    play_state->main_video_index = val.val.i;
	}
	REFCOUNT_RELEASE(actor);
    }


    play_state->play_speed = m_play_speed;
    play_state->audio_volume = m_audio_volume;
    play_state->audio_mute = m_audio_mute;

    play_state->playlist_index = m_playlist_idx;
    play_state->playlist_count = array_count(&m_playlist);

    if( m_sched->GetVal("play_time", val) ) {
	play_state->play_time = val.val.d;
    }

    if( m_sched->GetVal("play_paused", val) ) {
	play_state->play_paused = val.val.i;
    }

    play_state->play_eof = 0;

    for(idx=array_count(&m_play_tasks)-1; idx>=0; idx--) {
	PlayTask *play_task = *(PlayTask**)array_get(&m_play_tasks, idx);

	if( play_task->GetVal("time_duration", val)
	    && val.val.d > play_state->time_duration ) {
	    play_state->time_duration = val.val.d;
	}
	//if( play_task->GetVal("play_eof", val) && val.val.d ) {
	//    play_state->play_eof++;
	//}
    }
    
    /* hit eof iff all avis at eof. I used to see if all play_tasks
       were at eof, but some can't do that */
    //n = array_count(&m_play_tasks);
    //play_state->play_eof = play_state->play_eof == n;
    play_state->play_eof = play_state->time_duration > 0 
	&& play_state->play_time > play_state->time_duration;
    
    return 0;
}

int
GLPlayer::PlayListClear() {
    GLWORLDTHREADLOCK(-1);

    do {
	PlayStop();
	array_clear(&m_playlist);
	m_playlist_idx = 0;
	
	/* reset everything to defaults */
	m_play_speed = 1;
	
	GLPlayerActor *actor = (GLPlayerActor*)FindActor("video_01");
	if( actor ) {
	    LayoutSetMainVideo(actor);
	    REFCOUNT_RELEASE(actor);
	}

    } while(0);

    return 0;
}

int
GLPlayer::PlayListAppend(const char *path) {
    GLWORLDTHREADLOCK(-1);

    PlayListGroup *play_group;
    int err=-1;
    char *p;
    
    do {
	debug(DEBUG_INFO, 
	      ("GLPlayer::PlayListAppend m_playlist_idx=%d path=%s\n",
	       m_playlist_idx, path));
	
	if( !path || !*path ) {
	    m_playlist_idx++;
	    err = 0;
	    break;
	}

	if( m_playlist_idx >= array_count(&m_playlist) ) {
	    play_group = (PlayListGroup*)array_add(&m_playlist, 1);
	    assertb(play_group);
	    m_playlist_idx = array_count(&m_playlist)-1;
	    array_init(&play_group->paths, sizeof(char*), 1);
	    array_set_free(&play_group->paths, array_free_malloc_ptr_ptr, 0);
	}
	else {
	    play_group = (PlayListGroup*)array_get(&m_playlist, m_playlist_idx);
	}
	
	p = strdup(path);
	assertb(p);
	*(char**)array_add(&play_group->paths, 1) = p;

	err = 0;
    } while(0);

    return err;
}

int
GLPlayer::PlayListRotate() {
    GLWORLDTHREADLOCK(-1);

    PlayState ps;
    int err=-1;

    do {
	GetPlayState(&ps);

	if( m_play_speed < 0 && ps.play_time < 0) {
	    err = SetPlayListIndex(ps.playlist_index - 1);
	}
	else {
	    /* only rotate on eof */
	    if( !ps.play_eof ) {
		err = 0;
		break;
	    }
	    err = SetPlayListIndex(ps.playlist_index + 1);
	}
    } while(0);

    return err;
}

int
GLPlayer::GetPlayListCount() {
    GLWORLDTHREADLOCK(-1);

    return array_count(&m_playlist);
}

double
GLPlayer::GetPlayListIndexDuration(int idx) {
    PlayListGroup *play_group;
    double duration_max = 0;
    int i, path_idx;

    do {
	play_group = (PlayListGroup*)array_get(&m_playlist, idx);
	assertb(play_group);
	duration_max = 0;
	for(path_idx=0; path_idx<array_count(&play_group->paths); path_idx++) {
	    char *path = *(char**)array_get(&play_group->paths, path_idx);
	    char *ext = strrchr(path, '.');
	    if( ext ) ext++;

	    if( ext && strcasecmp(ext, "smi")==0 ) {
		/* ignore SMI duration */
	    }
	    else if ( ext && strcasecmp(ext, "avi")==0  ) {
		AVIHEADER avih;
		double d;
		
		i = AviEnumFile(path, AviEnumFileGetAviHeader, &avih);
		assertb(i > 0 );
		d = (double)avih.dwMicroSecPerFrame 
		    * avih.dwTotalFrames / 1e6;
		if( duration_max < d ) {
		    duration_max = d;
		}
	    }
	}
    } while(0);
    return duration_max;
}

int
GLPlayer::SetPlayListIndex(int idx) {
    GLWORLDTHREADLOCK(-1);

    int i, n, err=-1;
    PlayListGroup *play_group;
    GLPlayer::PlayState ps;

    do {
	n = array_count(&m_playlist);

	debug(DEBUG_INFO, 
	      ("GLPlayer::SetPlayListIndex idx=%d count=%d\n", idx, n));

	if( n <= 0 ) {
	    err = 0;
	    break;
	}

	PlayStop();
	m_play_state_cache_ok = 0;

	if( idx < 0 ) {
	    idx += n;
	}
	m_playlist_idx = idx % n;
	play_group = (PlayListGroup*)array_get(&m_playlist, m_playlist_idx);

	for(i=0; i<array_count(&play_group->paths); i++) {
	    char *path = *(char**)array_get(&play_group->paths, i);
	    char *ext = strrchr(path, '.');
	    if( ext ) ext++;
	    
	    
	    if( ext && strcasecmp(ext, "smi")==0 ) {
		AddSmi(path);
	    }
	    else if ( ext && strcasecmp(ext, "avi")==0  ) {
		AddAvi(path);
	    }
	}
	
	LayoutBox *box = m_layout_root ? m_layout_root->Find("video_main") : 0;
	LayoutSetMainVideo(box ? (GLPlayerActor*)box->GetData() : 0);
	Layout(0);
	UpdateAudio();

	GetPlayState(&ps);
	if( ps.play_speed < 0 ) {
	    SetPlayTime(ps.time_duration);
	}
	else {
	    SetPlayTime(0);
	}

#if 1
	// kludge - the main video can starve all the little one so
	// they never update.  So, I make sure everything gets a
	// chance to update once before continuing.
	for(int j=0; j<200 && m_sched->Poll(); j++);
#endif
	
	err = 0;
    } while(0);

    return err;
}

int
GLPlayer::SetSubvideoPriority(int priority) {
    GLWORLDTHREADLOCK(-1);

    m_subvideo_priority = priority;
    LayoutSetMainVideo(0);
    return 0;
}

int
GLPlayer::SetStatusText(const char *text) {
    GLWORLDTHREADLOCK(-1);

    GLPlayerActor *actor = (GLPlayerActor*)FindActor("status_text");
    if( actor ) {
	actor->SetVal("text", text);
	REFCOUNT_RELEASE(actor);
	UpdateRedraw();
    }
    
    return 0;
}
int
GLPlayer::DrawText(const char *str, int len, BoxUnit *width, BoxUnit *height, int measure_only) {
    GLWORLDTHREADLOCK(-1);

    int i;
    GLdouble dy, dy_newline;
    float llx, lly, llz, urx, ury, urz;

    if( len <= 0 ) {
	len = strlen(str);
    }

    do {
	assertb(m_font);
	m_font->BBox("M", -1, llx, lly, llz, urx, ury, urz);
	dy = ury - lly;
	dy_newline = dy *.2;
	
	glPushMatrix();
	
	if( !measure_only ) {
	    glTranslated(0, -dy, 0);
	}
	

	if( width && height ) {
	    *width = 0;
	    *height = 0;
	}

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while(len>0 && *str) {
	    i = strcspn(str, "\n");

	    if( width && height ) {
		m_font->BBox(str, i, llx, lly, llz, urx, ury, urz);
		float x = urx - llx;
		if( x > *width ) {
		    *width = x;
		}
		//*height += ury-lly;
		*height += dy;
	    }
	    
	    if( !measure_only ) {
		glPushMatrix();
		m_font->Render(str, i);
		glPopMatrix();
		assertb_gl();
		glTranslated(0, -dy, 0);
	    }

	    i++;
	    str += i;
	    len -= i;

	    if( len>0 && *str ) {
		if( height ) {
		    *height += dy_newline;
		}
		glTranslated(0, -dy_newline, 0);
	    }
	}

	glPopAttrib();

	glPopMatrix();
    } while(0);
    
    return 0;
}

