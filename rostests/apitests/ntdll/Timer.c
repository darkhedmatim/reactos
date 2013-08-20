/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtQueryTimerResolution and NtSetTimerResolution.
 * PROGRAMMER:      Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#define WIN32_NO_STATUS
#include <wine/test.h>
#include <ndk/ketypes.h>
#include <ndk/kefuncs.h>

#include <stdio.h>

START_TEST(TimerResolution)
{
    NTSTATUS Status;
    ULONG CurrentResolution;
    ULONG MinimumResolution;
    ULONG MaximumResolution;
    ULONG CurrentResolution2;

    /* Get the current timer resolution */
    Status = NtSetTimerResolution(0,        /* Ignored */
                                  FALSE,    /* Don't change resolution */
                                  &CurrentResolution);

    /*
     * If the timer resolution hasn't been changed for this process,
     * it returns STATUS_TIMER_RESOLUTION_NOT_SET
     */
    ok_hex(Status, STATUS_TIMER_RESOLUTION_NOT_SET);

    /*
     * Get the timer resolution limits and current timer resolution
     * using a different method
     */
    Status = NtQueryTimerResolution(&MinimumResolution,
                                    &MaximumResolution,
                                    &CurrentResolution2);

    /* This function should always return STATUS_SUCCESS */
    ok_hex(Status, STATUS_SUCCESS);

    /* The MinimumResolution should be higher than the MaximumResolution */
    ok(MinimumResolution >= MaximumResolution, "MaximumResolution higher than MinimumResolution!");

    /* These two values should be the same */
    ok_hex(CurrentResolution, CurrentResolution2);

    /*
     * Even if you give it invalid values,
     * NtSetTimerResolution will return STATUS_SUCCESS,
     * but it will not change the resolution.
     */
    Status = NtSetTimerResolution(MinimumResolution + 1,
                                  TRUE,
                                  &CurrentResolution);
    ok_hex(Status, STATUS_SUCCESS);
    printf("Current resolution: %d ; minimum resolution: %d\n", CurrentResolution, MinimumResolution);
    ok(CurrentResolution <= MinimumResolution, "Current resolution: %d became too high! (minimum resolution: %d)\n", CurrentResolution, MinimumResolution);

    Status = NtSetTimerResolution(MaximumResolution - 1,
                                  TRUE,
                                  &CurrentResolution);
    ok_hex(Status, STATUS_SUCCESS);
    printf("Current resolution: %d ; maximum resolution: %d\n", CurrentResolution, MaximumResolution);
    ok(CurrentResolution >= MaximumResolution, "Current resolution: %d became too low! (maximum resolution: %d)\n", CurrentResolution, MaximumResolution);

    /* Get the current timer resolution */
    Status = NtSetTimerResolution(0,        /* Ignored */
                                  FALSE,    /* Don't change resolution */
                                  &CurrentResolution);

    /* Since we have changed the resolution earlier, it returns STATUS_SUCCESS. */
    ok_hex(Status, STATUS_SUCCESS);
}
