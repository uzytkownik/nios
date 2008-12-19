#include "iapi.h"

#include <atomic.h>

#define IAPI_MAGIC   0x1AB1
#define IAPI_VERSION 0x0001
#define IAPI_RMAGIC ((IAPI_MAGIC << 16) | IAPI_VERSION)

void
iapi_init ()
{

}

struct iapi *
iapi_new (struct iapi *self, iapi_free free)
{
  self->magic = IAPI_RMAGIC;
  ATOMIC_SET (&self->ref, 0);
  self->free = free;
  return self;
}

int
iapi_check (struct iapi *self)
{
  int ref;

  if (!self)
    return 0;
  
  ATOMIC_GET (&self->ref, ref);
  
  return self->magic == IAPI_RMAGIC && ref > 0;
}

void
iapi_ref (struct iapi *self)
{
  ATOMIC_INC (&self->ref);
}

void
iapi_unref (struct iapi *self)
{
  int ref;
  
  ATOMIC_DEC (&self->ref);

  ATOMIC_GET (&self->ref, ref);

  if (ref == 0 && self->free)
    self->free (self);
}
