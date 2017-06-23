#include <stdlib.h>
#include <string.h>

#include "task.h"

#include "gkvid/config.h"

struct MsgQueueNode {
    MsgQueue *queue ;
    MsgQueueSerial *serial;
    TaskQueueDir dir;
};

//---------------------------------------------------------------------
Task::Task(const char *name) {
    m_task_name = 0;
    if( name ) {
	SetTaskName(name);
    }
    array_init(&m_queue, sizeof(MsgQueueNode), 0);
    m_sched = 0;
}

int
Task::Init() {
    return 0;
}

Task::~Task() {
    int i;
    if( m_task_name ) {
	free(m_task_name);
	m_task_name = 0;
    }
    for(i=0; i<array_count(&m_queue); i++) {
	MsgQueueNode *pq = (MsgQueueNode*)array_get(&m_queue, i);
	REFCOUNT_RELEASE(pq->queue);
	if( pq->serial ) {
	    delete(pq->serial);
	    debug_leak_delete(pq->serial);
	    pq->serial = 0;
	}
    }
    array_clear(&m_queue);
}

int
Task::SetTaskName(const char *name) {
    if( m_task_name ) {
	free(m_task_name);
    }
    m_task_name = strdup(name);
    return 0;
}

const char* 
Task::GetTaskName() {
    return m_task_name;
}

int
Task::GetQueueCount() {
    return array_count(&m_queue);
}

MsgQueue*
Task::GetQueue(int queue_idx, TaskQueueDir *rw, MsgQueueSerial **serial) {
    MsgQueueNode *pq;
    pq = (MsgQueueNode*)array_get(&m_queue, queue_idx);
    if( !pq ) return 0;
    if( rw ) {
	*rw = pq->dir;
    }
    if( serial ) {
	*serial = pq->serial;
    }
    if( !pq->queue ) {
	return 0;
    }

    pq->queue->AddRef();
    return pq->queue;
}

MsgQueueMsg*
Task::GetNextInputMsg(int *queue_idx) {
    TaskQueueDir dir;
    int i;

    for(i=0; i<GetQueueCount(); i++) {
	MsgQueue *queue=0;
	MsgQueueMsg *msg=0;
	MsgQueueSerial *serial=0;
	do {
	    queue = GetQueue(i, &dir, &serial);
	    if( !queue ) { break; }
	    if( dir != QUEUE_IN ) { break; }
	    assertb(serial);
	    msg = queue->MsgGetNext(*serial);
	} while(0);
	if( queue ) {
	    queue->Release();
	}
	if( msg ) {
	    if( queue_idx ) { 
		*queue_idx = i; 
	    }
	    return msg;
	}
    }
    return 0;
}

int
Task::ConnectQueue(MsgQueue *queue, TaskQueueDir rw, int queue_idx) {
    MsgQueueNode *pq;
    int i, n, err=-1;

    do {
	n = array_count(&m_queue);
	if( queue_idx < 0 ) {
	    queue_idx = n;
	}
	if( queue_idx >= n ) {
	    array_add(&m_queue, queue_idx-n+1);
	}
	pq = (MsgQueueNode*)array_get(&m_queue, queue_idx);
	assertb(pq);
	REFCOUNT_RELEASE(pq->queue);
	if( pq->serial ) {
	    debug_leak_delete(pq->serial);
	    delete pq->serial;
	    pq->serial = 0;
	}
	queue->AddRef();
	pq->queue = queue;
	pq->dir = rw;
	pq->serial = new MsgQueueSerial();
	debug_leak_create_assertb(pq->serial);

	i = queue->ConnectTask(this, rw);
	assertb(i>=0);

	err = 0;
    } while(0);
    return err ? err : queue_idx;
}

int
Task::PollReady() {
    return 1;
}

int
Task::Poll() {
    return 0;
}

TaskScheduler*
Task::GetScheduler() {
    if( m_sched ) {
	m_sched->AddRef();
    }
    return m_sched;
}


int
Task::SetScheduler(TaskScheduler *sched, int priority) {
    m_sched = sched;
    m_priority = priority;
    return 0;
}

int
Task::SetPriority(int priority) {
    m_priority = priority;
    if( m_sched ) {
	m_sched->SetPriority(this, priority);
    }
    return 0;
}

int
Task::GetPriority() {
    return m_priority;
}


