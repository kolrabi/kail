//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2002 by Denton Woods
// Copyright (C) 2002 Nelson Rush.
// Last modified: 05/18/2002
//
// Filename: src-ILUT/src/ilut_x11.c
//
// Description: X11 Pixmap and XImage binding functions (with XShm)
//
//-----------------------------------------------------------------------------

/*
** This file was created by Jesse Maurais Wed April 4, 2007
** Contact: www.jessemaurais@gmail.com
**
**
** This patch to the Devil Tookit binds to the X Windows System Version 11,
** using client-side XImages and server-side Pixmaps, with support for both
** ZPixmaps and the insane XYPixmap format. (Depends on the X server)
**
** If the XShm extension to X11 is present at the time of ./configure then
** support for shared memory XImages and Pixmaps is also compiled in. Note
** that "shared" does not mean Devil and X are sharing the same memory space.
** This is not possible, because both libraries make byte-for-byte copies
** of their data. It means that memory is part of an inter-process memory
** segment (see XShm spec).
**
** TODO
** 1) Assumed the display depth is 24 bits (the most common) but there should
** be a check, and iXConvertImage should handle this properly. I don't think
** this should be difficult to modify from whats here.
** 2) It would be nice to convert from an XImage back to a Devil image for
** saving changes. Would be seful for an interactive image editor.
** 3) Possibly some additional OpenGL bindings for GLX Pbuffers.
**
** FYI
** It was a long night figuring out the under-documented XYPixmap format.
*/


#include "ilut_internal.h"
#ifdef ILUT_USE_X11

/*
int bits; // bits per pixel
int field;  // bits per channel
int bytes;  // bytes per pixel
int grain;  // bytes per line
int width;  // pixels per line
int height; // number of lines
ILpal* palette; // for indexed colors
char* data; // pointer to pixels



void iXGrabImage( ILimage * img )
{
  bits     = img->Bpp*8;  // bits per pixel
  field    = img->Bpc;  // bits per channel
  bytes    = img->Bpp;  // bytes per pixel
  grain    = img->Bps;  // bytes per line
  width    = img->Width;
  height   = img->Height;
  palette  = &img->Pal;
  data     = (char*)img->Data;
}


Bool iXGrabCurrentImage(void) {
  iLockState();
  ILimage *ilutCurImage = iLockCurImage();
  iUnlockState();

  if (!ilutCurImage) {
    return False;
  }

  iXGrabImage(ilutCurImage);
  iUnlockImage(ilutCurImage);
  return True;
}
*/

void iXConvertImage( ILimage *ilutCurImage, XImage * img )
{
  ILuint x,y,z;
  int sX,dX;
  int sY,dY;
  int sZ,dZ;
  int plane;

  ILimage * tmp;

  switch ( img->byte_order )
  {
   case LSBFirst:
    tmp = iConvertImage( ilutCurImage,IL_BGR,IL_UNSIGNED_BYTE );
    break;
   case MSBFirst:
    tmp = iConvertImage( ilutCurImage,IL_RGB,IL_UNSIGNED_BYTE );
    break;
   default:
    return;
  }

  if ( !tmp ) {
    return;
  }

  switch ( img->format ) {
    case ZPixmap:
      for ( y = 0; y < tmp->Height; y ++ ) {
        dY = y * img->bytes_per_line;
        sY = y * tmp->Bps;

        for ( x = 0; x < tmp->Width; x ++ ) {
          dX = x * img->bits_per_pixel / 8;
          sX = x * tmp->Bpp;

          for ( z = 0; z < tmp->Bpp; z ++ ) {
            img->data[dX+dY+z] = tmp->Data[sX+sY+z];
          }
        }
      }
      break;

    case XYPixmap:
      for ( y = 0; y < tmp->Height; y ++ ) {
        sY = y * tmp->Bps;

        for ( x = 0; x < tmp->Width; x ++ ) {
          sX = x * tmp->Bpp;

          for ( z = 0; z < tmp->Bpp*8; z ++ ) {
            sZ = z / 8;
            dZ = z % 8;

            if ( tmp->Data[sY+sX+sZ] & ( 1 << dZ ) ) {
              plane = tmp->Bpp*8 - z - 1;

              sZ = x % 8;
              dX = x / 8;
              dY = y * img->bytes_per_line;
              dZ = plane * img->bytes_per_line * tmp->Height;

              img->data[dZ+dY+dX] |= 1 << sZ;
            }
          }
        }
      }
    break;

    default:
      iSetError( ILUT_NOT_SUPPORTED );
  }

  iCloseImage( tmp );
}

ILboolean ilutXInit(void) {
  return IL_TRUE;
}

XImage * ILAPIENTRY ilutXCreateImage_( ILimage *ilutCurImage, Display * dpy ) {
  Visual * vis;
  XImage * img;
  char * buffer;

  if (!ilutCurImage) 
    return NULL;

  buffer = (char*)ialloc( ilutCurImage->Width * ilutCurImage->Height * 4 );
  if (!buffer) {
    return NULL;
  }

  vis = CopyFromParent;
  img = XCreateImage( dpy,vis, 24,ZPixmap,0,buffer,ilutCurImage->Width,ilutCurImage->Height,8,0 );
  if (!img) {
    ifree(buffer);
    iUnlockImage(ilutCurImage);
    return NULL;
  }

  iXConvertImage( ilutCurImage, img );
  return img;
}

XImage * ilutXCreateImage( Display * dpy ) {
  iLockState();
  ILimage *ilutCurImage = iLockCurImage();
  iUnlockState();

  XImage *Result = ilutXCreateImage_(ilutCurImage, dpy);
  iUnlockImage(ilutCurImage);
  return Result;
}

Pixmap ilutXCreatePixmap_( ILimage *ilutCurImage, Display * dpy, Drawable draw ) {
  XImage * img;
  GC gc;
  Pixmap pix;

  img = ilutXCreateImage_( ilutCurImage, dpy );

  if (!img) {
    return None;
  }

  gc = DefaultGC(dpy,DefaultScreen(dpy));
  if (!gc) {
    XDestroyImage( img );
    return None;
  }

  pix = XCreatePixmap( dpy,draw, img->width, img->height, 24 );
  if (!pix ) {
    XDestroyImage( img );
    return None;
  }

  XPutImage( dpy,pix,gc,img, 0,0,0,0, img->width, img->height );
  XDestroyImage( img );
  
  return pix;
}

Pixmap ILAPIENTRY ilutXCreatePixmap( Display * dpy, Drawable draw ) {
  iLockState();
  ILimage *ilutCurImage = iLockCurImage();
  iUnlockState();

  Pixmap pix = ilutXCreatePixmap_(ilutCurImage, dpy, draw);
  iUnlockImage(ilutCurImage);

  return pix;
}

XImage * ILAPIENTRY ilutXLoadImage( Display * dpy, char * filename_ )
{
#ifdef _UNICODE
  ILchar *filename = iWideFromMultiByte(filename_);
#else
  ILchar *filename = iStrDup(filename_);
#endif

  iLockState();
  ILimage *ilutCurImage = iLockCurImage();
  ILimage* Temp = iNewImage(1,1,1, 1,1);
  Temp->io = ilutCurImage->io;
  Temp->io.handle = NULL;
  iUnlockImage(ilutCurImage);
  iUnlockState();

  if (!iLoad(Temp, IL_TYPE_UNKNOWN, filename)) {
    ifree(filename);
    iCloseImage(Temp);
    return NULL;
  }

  ifree(filename);
  XImage *Result = ilutXCreateImage_( Temp, dpy );
  iCloseImage(Temp);

  return Result;
}

Pixmap ILAPIENTRY ilutXLoadPixmap( Display * dpy, Drawable draw, char * filename_ ) {
#ifdef _UNICODE
  ILchar *filename = iWideFromMultiByte(filename_);
#else
  ILchar *filename = iStrDup(filename_);
#endif

  iLockState();
  ILimage *ilutCurImage = iLockCurImage();
  ILimage* Temp = iNewImage(1,1,1, 1,1);
  Temp->io = ilutCurImage->io;
  Temp->io.handle = NULL;
  iUnlockImage(ilutCurImage);
  iUnlockState();

  if (!iLoad(Temp, IL_TYPE_UNKNOWN, filename)) {
    ifree(filename);
    iCloseImage(Temp);
    return None;
  }

  ifree(filename);
  Pixmap Result = ilutXCreatePixmap_( Temp, dpy, draw );
  iCloseImage(Temp);

  return Result;
}

#ifdef ILUT_USE_XSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>


static XImage * iXShmCreateImage( ILimage *ilutCurImage, Display * dpy, XShmSegmentInfo * info ) {
  Visual * vis;
  XImage * img;
  int size, format;

  if (!ilutCurImage) {
    iSetError(IL_ILLEGAL_OPERATION);
    return NULL;
  }

  // Get server supported format
  format = XShmPixmapFormat( dpy );

  // Create a shared image
  vis = CopyFromParent;
  img = XShmCreateImage( dpy,vis, 24,format,NULL,info,ilutCurImage->Width,ilutCurImage->Height );

  if (!img) {
    iUnlockImage(ilutCurImage);
    return NULL;
  }

  // Create shared memory

  size = img->bytes_per_line * img->height;

  info->shmid = shmget( IPC_PRIVATE, size, IPC_CREAT | 0666 );
  info->shmaddr = img->data = shmat( info->shmid, 0, 0 );
  info->readOnly = False;

  // Attach to server

  XShmAttach( dpy,info );

  // Copy image pixels to shared memory

  iXConvertImage( ilutCurImage, img );
  iUnlockImage(ilutCurImage);

  return img;
}

XImage * ILAPIENTRY ilutXShmCreateImage( Display * dpy, XShmSegmentInfo * info )
{
  ILimage *ilutCurImage;
  XImage  *Result;

  iLockState();
  ilutCurImage = iLockCurImage();
  iUnlockState();

  Result = iXShmCreateImage( ilutCurImage, dpy, info );
  iUnlockImage(ilutCurImage);

  return Result;
}

void ILAPIENTRY ilutXShmDestroyImage( Display * dpy, XImage * img, XShmSegmentInfo * info )
{
  XShmDetach( dpy,info );
  XDestroyImage( img );
  XFlush( dpy );

  shmdt( info->shmaddr );
  shmctl( info->shmid, IPC_RMID, 0 );
}

static Pixmap iXShmCreatePixmap( ILimage *ilutCurImage, Display * dpy, Drawable draw, XShmSegmentInfo * info )
{
  Pixmap pix;
  XImage*img;

  // Create a dumby image

  img = iXShmCreateImage( ilutCurImage, dpy,info );
  if (!img) {
    return None;
  }

  // Use the same memory segment in the pixmap

  pix = XShmCreatePixmap( dpy,draw, info->shmaddr,info,img->width,img->height,24 );
  if (!pix) {
    ilutXShmDestroyImage( dpy,img,info );
    return None;
  }

  // Riddance to the image

  XDestroyImage( img );

  return pix;
}

Pixmap ILAPIENTRY ilutXShmCreatePixmap( Display * dpy, Drawable draw, XShmSegmentInfo * info )
{
  ILimage *ilutCurImage;
  Pixmap  Result;

  iLockState();
  ilutCurImage = iLockCurImage();
  iUnlockState();

  Result = iXShmCreatePixmap( ilutCurImage, dpy, draw, info );
  iUnlockImage(ilutCurImage);

  return Result;
}


void ILAPIENTRY ilutXShmFreePixmap( Display * dpy, Pixmap pix, XShmSegmentInfo * info )
{
  XShmDetach( dpy,info );
  XFreePixmap( dpy,pix );
  XFlush( dpy );

  shmdt( info->shmaddr );
  shmctl( info->shmid, IPC_RMID, 0 );
}



XImage * ILAPIENTRY ilutXShmLoadImage( Display * dpy, char* filename_, XShmSegmentInfo * info )
{
#ifdef _UNICODE
  ILchar *filename = iWideFromMultiByte(filename_);
#else
  ILchar *filename = iStrDup(filename_);
#endif
  ILimage *ilutCurImage;
  ILimage* Temp;
  XImage * Result;

  iLockState();
  ilutCurImage = iLockCurImage();
  Temp = iNewImage(1,1,1, 1,1);
  Temp->io = ilutCurImage->io;
  Temp->io.handle = NULL;
  iUnlockImage(ilutCurImage);
  iUnlockState();

  if (!iLoad(Temp, IL_TYPE_UNKNOWN, filename)) {
    iCloseImage(Temp);
    ifree(filename);
    return NULL;
  }

  ifree(filename);

  Result = iXShmCreateImage( Temp,dpy,info );
  iCloseImage(Temp);

  return Result;
}
 


Pixmap ILAPIENTRY ilutXShmLoadPixmap( Display * dpy, Drawable draw, char* filename_, XShmSegmentInfo * info )
{
#ifdef _UNICODE
  ILchar *filename = iWideFromMultiByte(filename_);
#else
  ILchar *filename = iStrDup(filename_);
#endif
  ILimage *ilutCurImage;
  ILimage* Temp;
  Pixmap   Result;

  iLockState();
  ilutCurImage = iLockCurImage();
  Temp = iNewImage(1,1,1, 1,1);
  Temp->io = ilutCurImage->io;
  Temp->io.handle = NULL;
  iUnlockImage(ilutCurImage);
  iUnlockState();

  if (!iLoad(Temp, IL_TYPE_UNKNOWN, filename)) {
    iCloseImage(Temp);
    ifree(filename);
    return None;
  }

  ifree(filename);

  Result = iXShmCreatePixmap( Temp, dpy, draw, info );
  iCloseImage(Temp);

  return Result;

/*  
#ifdef _UNICODE
  ILchar *filename = iWideFromMultiByte(filename_);
#else
  ILchar *filename = iStrDup(filename_);
#endif

  iBindImageTemp();
  if (!ilLoadImage(filename)) {
    ifree(filename);
    return None;
  }
  ifree(filename);
  return ilutXShmCreatePixmap( dpy,draw,info );

  ILimage *ilutCurImage;
  Pixmap  Result;

  iLockState();
  ilutCurImage = iLockCurImage();
  iUnlockState();

  Result = iXShmCreatePixmap( ilutCurImage, dpy, info );
  iUnlockImage(ilutCurImage);

  return Result;
  */
}

#endif//ILUT_USE_XSHM
#endif//ILUT_USE_X11

