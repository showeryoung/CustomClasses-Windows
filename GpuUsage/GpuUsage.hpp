#pragma once
#include "d3dkmt.h"
// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);               \
    
//临界区封装
class SectionLock {
private:
    // make copy constructor and assignment operator inaccessible
    DISALLOW_COPY_AND_ASSIGN(SectionLock);
    CRITICAL_SECTION _critical_section;

public:
    SectionLock() {
        InitializeCriticalSection(&_critical_section);
    };

    ~SectionLock() {
        DeleteCriticalSection(&_critical_section);
    };

    void Lock() {
        EnterCriticalSection(&_critical_section);
    };

    void Unlock() {
        LeaveCriticalSection(&_critical_section);
    };
};
//使用封装临界区的自动锁
class AutoLock {
private:
    // make copy constructor and assignment operator inaccessible
    DISALLOW_COPY_AND_ASSIGN(AutoLock);

protected:
    SectionLock * _section_lock;

public:
    AutoLock(SectionLock * plock) {
        _section_lock = plock;
        _section_lock->Lock();
    };

    ~AutoLock() {
        _section_lock->Unlock();
    };
};

class GpuUsage
{
private:
    // make copy constructor and assignment operator inaccessible
    GpuUsage(const GpuUsage &refGpuUsage);
    GpuUsage &operator=(const GpuUsage &refGpuUsage);
public:
    GpuUsage();
    ~GpuUsage();

public:
    bool initializeAdapter();
    void uninitalizeAdapter();
    void updateUsage();
    bool isGpuIdle();

    //private:
    bool startMonitor();
    void endMonitor();
    void checkBusyState(const float current_gpu);

private:
    bool _is_initialized;
    SectionLock _usage_lock;
    HMODULE _gdi32_hmodule;
    PFND3DKMT_QUERYSTATISTICS XD_D3DKMTQueryStatistics;
    D3DKMT_QUERYSTATISTICS _query_statistics;
    float _current_usage;
    UINT _timer_ID;
    LARGE_INTEGER _performance_counter;
    LONGLONG _running_time;
};