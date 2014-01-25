#include "MediaTimer.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <MMSystem.h>

#pragma comment(lib, "winmm.lib")

MediaTimer::MediaTimer(LPTIMECALLBACK callback_func)
    :_timer_id(0)
    ,_callback_func(callback_func)
    ,_res_time(5)
{
}

MediaTimer::~MediaTimer()
{
    if(0 != _timer_id) {
        endTimer();
    }
}

void MediaTimer::beginTimer(
    unsigned int delay_time,
    unsigned int res_time,
    unsigned long user_para)
{
    if(0 != _timer_id) {
        return;
    }
    _res_time = res_time>0U ? res_time : _res_time;
    timeBeginPeriod(_res_time);
    _timer_id = timeSetEvent(delay_time, _res_time, _callback_func, user_para, TIME_PERIODIC);
}

void MediaTimer::beginOnceTimer(
    unsigned int delay_time,
    unsigned int res_time,
    unsigned long user_para)
{
    if(0 != _timer_id) {
        return;
    }
    _res_time = res_time;
    timeBeginPeriod(_res_time);
    _timer_id = timeSetEvent(delay_time, _res_time, _callback_func, user_para, TIME_ONESHOT);
}

void MediaTimer::updateCallback(LPTIMECALLBACK callback_func)
{
    if(0 == _timer_id) {
        _callback_func = callback_func;
        return;
    }
}

void MediaTimer::endTimer()
{
    timeKillEvent(_timer_id);
    timeEndPeriod(_res_time);
    _timer_id = 0;
}
