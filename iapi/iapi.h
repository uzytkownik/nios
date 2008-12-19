#ifndef IAPI_IAPI_H
#define IAPI_IAPI_H

struct iapi;

typedef void (*iapi_free)(struct iapi *iapi);

struct iapi
{
  int magic;
  iapi_free free;
  volatile int ref;
};

void iapi_init ();
struct iapi *iapi_new (struct iapi *self, iapi_free free);
int iapi_check (struct iapi *self);
void iapi_ref (struct iapi *self);
void iapi_unref (struct iapi *self);

#endif
