#include "waitsome.h"

#include "assert.h"
#include "success_or_die.h"

void wait_or_die
  ( gaspi_segment_id_t segment_id
  , gaspi_notification_id_t notification_id
  , gaspi_notification_t expected
  )
{
  gaspi_notification_id_t id;

  SUCCESS_OR_DIE
    (gaspi_notify_waitsome (segment_id, notification_id, 1, &id, GASPI_BLOCK));

  ASSERT (id == notification_id);

  gaspi_notification_t value;

  SUCCESS_OR_DIE (gaspi_notify_reset (segment_id, id, &value));
  gaspi_rank_t iProc;
  SUCCESS_OR_DIE(gaspi_proc_rank(&iProc));
  ASSERT (value == expected);
}

int wait_or_die_limited
  ( gaspi_segment_id_t segment_id
  , gaspi_notification_id_t notification_id
  , gaspi_notification_t expected
  , gaspi_timeout_t timeout
  )
{

  gaspi_notification_id_t id;
  gaspi_return_t ret;

  if ( ( ret =
         gaspi_notify_waitsome (segment_id, notification_id, 1, &id, timeout)
       ) == GASPI_SUCCESS
     )
  {

    gaspi_notification_t value;

    SUCCESS_OR_DIE (gaspi_notify_reset (segment_id, id, &value));

    return 1;
  }
  else
  {
    ASSERT (ret != GASPI_ERROR);

    return 0;
  }
}
