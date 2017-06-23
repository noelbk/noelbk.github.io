/*------------------------------------------------------------------*/
/* 
   File: gk/src/gkvid/task.h
   Author: Noel Burton-Krahn <noel@burton-krahn.com>
   Created: Aug 01, 2005
   
   Tasks, Messages, Queues, and the Scheduler

  I have to read 4 cameras and audio, save to avi, decompress video,
  display it on the screen, and do motion detection.  The read and
  save must happen in real time, the display can lag behind.

  A Task is polled by the TaskScheduler and processes
  MsgQueueMsg's from input to output MsgQueue's.  For
  example: A filter task would move from one input to one output
  queue.  A source would read from a file and put on an output queue,
  etc.

  A program connects tasks to message queues, assigns tasks priority in a
  scheduler, and polls the scheduler.

*/

#ifndef TASK_H_INCLUDED
#define TASK_H_INCLUDED

#include "gkvid/taskscheduler.h"

typedef enum {
    QUEUE_NONE=0
    ,QUEUE_IN
    ,QUEUE_OUT
} TaskQueueDir;
    
#include "gkvid/msgqueue.h"

#include "bklib/mstime.h"
typedef mstime_t TaskTime;

class TaskScheduler;
class MsgQueue;
class MsgQueueSerial;
class MsgQueueMsg;

/*---------------------------------------------------------------------*/
class Task : public ValBag {
public:
    Task(const char *name=0);
    virtual int Init();

    virtual int SetTaskName(const char *name);
    virtual const char* GetTaskName();

    virtual int GetQueueCount();
    virtual MsgQueue* GetQueue(int queue_idx, TaskQueueDir *rw=0, MsgQueueSerial **serial=0);
    // returns queue_idx or -1 on error
    virtual int ConnectQueue(MsgQueue *queue, TaskQueueDir rw, int queue_idx=-1);
    virtual int PollReady();
    virtual int Poll();
    virtual MsgQueueMsg* GetNextInputMsg(int *queue_idx=0);
    virtual TaskScheduler* GetScheduler();
    virtual int SetScheduler(TaskScheduler *sched, int priority);
    virtual int SetPriority(int priority);
    virtual int GetPriority();

    TaskTime m_poll_next;

protected:
    virtual ~Task();

    TaskScheduler *m_sched;
    int m_priority;
    char *m_task_name;

    array_t m_queue;
    array_t m_queue_serial;
};

#endif // TASK_H_INCLUDED
