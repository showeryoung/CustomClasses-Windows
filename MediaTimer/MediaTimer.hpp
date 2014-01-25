#pragma once

//Timer callback
#ifndef LPTIMECALLBACK
typedef void (__stdcall TIMECALLBACK)(\
    unsigned int uTimerID,\
    unsigned int uMsg,\
    unsigned long dwUser,\
    unsigned long dw1,\
    unsigned long dw2);
typedef TIMECALLBACK *LPTIMECALLBACK;
#endif

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);               \
    void operator=(const TypeName&)
class MediaTimer
{
private:
    DISALLOW_COPY_AND_ASSIGN(MediaTimer);
public:
    MediaTimer(LPTIMECALLBACK callback_func);
    ~MediaTimer();

    void beginTimer(
        unsigned int delay_time,
        unsigned int res_time,
        unsigned long user_para);
    void beginOnceTimer(
        unsigned int delay_time,
        unsigned int res_time,
        unsigned long user_para);
    void updateCallback(LPTIMECALLBACK callback_func);
    void endTimer();

private:
    unsigned int _timer_id;
    LPTIMECALLBACK _callback_func;
    unsigned int _res_time;
};
