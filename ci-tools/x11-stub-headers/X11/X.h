/* minimal X.h stub for host smoke build (no real X11) */
#ifndef X_H
#define X_H
#include <sys/types.h>
typedef unsigned long XID;
typedef unsigned long Mask;
typedef unsigned long Atom;
typedef unsigned long VisualID;
typedef XID Window;
typedef XID Drawable;
typedef XID Font;
typedef XID Pixmap;
typedef XID Cursor;
typedef XID Colormap;
typedef XID GContext;
typedef XID KeySym;
#ifndef Bool
typedef int Bool;
#endif
#ifndef Status
typedef int Status;
#endif
typedef unsigned long Time;
typedef unsigned char KeyCode;
#define True 1
#define False 0
#define None 0L
#define CopyFromParent 0L
#define CurrentTime 0L
#define NoSymbol 0L
#define AllPlanes ((unsigned long)~0L)
#define QueuedAlready 0
#define QueuedAfterReading 1
#define QueuedAfterFlush 2
typedef struct _XGC *GC;
struct _XDisplay;
typedef struct _XDisplay Display;
#define ZPixmap 2
#define XYBitmap 1
#define XYPixmap 0
#define AllocNone 0
#define InputOutput 1
#define Success 0
#define BadAccess 6
#define BadShmSeg 13
#define StaticGray 0
#define StaticColor 2
#define TrueColor 4
typedef struct { unsigned char response_type; Bool send_event; Display *display; long serial; unsigned char code; } XErrorEventStub;
typedef struct { int type; unsigned long serial; Bool send_event; Display *display; Window window; int state; KeySym keysym; } XKeyEventStub;
typedef struct { unsigned long flags; unsigned long functions; } XSizeHintsStub;
typedef struct { int max_keypermod; KeyCode *modifiermap; } XModifierKeymapStub;
typedef struct { short red, green, blue; char flags; char pad; } XColorStub;
typedef struct { unsigned long pixel; unsigned short red, green, blue; char flags; char pad; } XColor2;
typedef struct { int nmbs; char **mb; } XmbTextStub;
#endif
