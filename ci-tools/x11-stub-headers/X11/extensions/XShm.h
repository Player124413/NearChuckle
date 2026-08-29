/* minimal XShm.h stub for host smoke build (no real X11) */
#ifndef XSHM_H
#define XSHM_H
#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
typedef struct {
    int shmidx_unused;
    long shmseg;
    void *shmid_pad;
} XShmSegmentInfoPad;
typedef struct {
    int shmid;
    char *shmaddr;
    Bool readOnly;
} XShmSegmentInfo;
#define ShmCompletion 0
#define ShmRequestEvent 0
#endif
