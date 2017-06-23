/*------------------------------------------------------------------*/
/* 
   File: gk/src/gkvid/glplayer.h
   Author: Noel Burton-Krahn <noel@burton-krahn.coma>
   Created: Sep, 2005

   The main player object.  This manages a playlist of avi and smi
   files, and plays using OpenGL.
*/

#ifndef GLPLAYER_H_INCLUDED
#define GLPLAYER_H_INCLUDED

#include "gkvid/glworld.h"
#include "gkvid/glactor.h"
#include "gkvid/playtask.h"
#include "gkvid/layoutbox.h"
#include "gkvid/glsprite.h"
#include "gkvid/glimage.h"
#include "gkvid/glfont.h"

#include "bklib/thread.h"

//--------------------------------------------------------------------
class GLPlayer;

class GLPlayerActor : public GLActor {
public:
    GLPlayerActor();
    virtual ~GLPlayerActor();
    virtual int            Init(GLPlayer *player, const char *name);
    virtual GLPlayer*      GetPlayer();
    virtual int            PlayStop();
    virtual int            ConnectQueue(MsgQueue *queue);
protected:
};

//--------------------------------------------------------------------
class GLPlayer : public GLWorld {
public:
    struct PlayState {
	int play_paused;
	int play_playing;
	TaskTime play_time;
	TaskTime time_duration;
	double play_speed;
	int play_eof;
	int playlist_index;
	int playlist_count;
	double audio_volume;
	int audio_mute;
	int main_video_index;
	int subvideo_priority;
    };

    GLPlayer();
    ~GLPlayer();
    virtual int Init();
    virtual int Close();
    virtual int CreateControls();
    virtual int DeleteControls();
    virtual int LoadControlImages();
    
    virtual int AddSmi(const char *smi_path);
    virtual int AddAvi(const char *avi_path);
    virtual int PlayStart();
    virtual int PlayStop();
    virtual int PlayPause(int pause=1);

    virtual int SetPlayTime(TaskTime t);
    virtual int GetPlayState(PlayState *play_state);

    // set play speed as a multiple of real time, 1=realtime, 2=ffwd, -1=reverse, .5=slow mo, etc
    virtual int SetPlaySpeed(double play_speed=1);

    virtual int SetAudioMute(int mute=1);
    // set audio volume, in range [0..1], and unmute
    virtual int SetAudioVolume(double volume);
    virtual int UpdateAudio();

    virtual int Draw();
    virtual int Layout(GLTime dt);
    virtual int LayoutSetMainVideo(GLPlayerActor *actor);
    
    virtual int PlayListClear();
    virtual int PlayListAppend(const char *path);
    virtual int GetPlayListCount();
    virtual int SetPlayListIndex(int i);
    virtual double GetPlayListIndexDuration(int idx);

    virtual int AddPlayTask(PlayTask *play_task);
    virtual int RemovePlayTask(PlayTask *play_task);

    virtual int SetSubvideoPriority(int priority);
    virtual int SetStatusText(const char *text);

    virtual int DrawText(const char *str, int len, BoxUnit *w=0, BoxUnit *h=0, int measure_only=0);

protected:
    static  void* PlayThread_s(void *arg);
    virtual void* PlayThread();
    virtual int PlayListRotate();
    
    thread_t *m_play_thread;
    array_t  m_play_tasks;
    int      m_play_thread_active;
    int m_video_name_idx;
    int m_audio_name_idx;

    int m_playlist_idx;
    array_t m_playlist;

    int m_main_video_idx;
    double m_play_speed;
    int m_play_paused;
    double m_audio_volume;
    int m_audio_mute;

    int m_play_state_cache_ok;
    PlayState m_play_state_cache;
    
    int m_subvideo_priority;
    GLFont *m_font;
};

/* max 32x play speed */
#define GLPLAYER_PLAY_SPEED_MAX_POWER2 (6)
#define GLPLAYER_PLAY_SPEED_MAX (1<<GLPLAYER_PLAY_SPEED_MAX_POWER2)

#endif // GLPLAYER_H_INCLUDED
