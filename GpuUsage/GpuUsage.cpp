#pragma once
#include "GpuUsage.hpp"
#include <stdio.h>
#include <MMSystem.h>
#pragma comment(lib, "winmm.lib")

#define kIsDebuging FALSE
#if kIsDebuging
#define PRINTF_DEBUG_MSG(msg) \
    printf("GpuUsage:%s.\n", msg)
#else
#define PRINTF_DEBUG_MSG(msg)
#endif
// GPU使用率空闲以及忙碌阈值
const float kIdleThreshold = 0.5f;
const float kBusyThreshold = 0.7f;

//操作系统版本是否为XP或更旧
bool isXPorEarlier()
{
    OSVERSIONINFO os_version;
    bool is_XP_or_later = FALSE;
    memset(&os_version, 0, sizeof(OSVERSIONINFO));
    os_version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&os_version);
    return (os_version.dwMajorVersion<6) ? true : false;
}

GpuUsage::GpuUsage()
    :_is_initialized(false)
    ,_usage_lock()
    ,_query_statistics()
    ,XD_D3DKMTQueryStatistics(NULL)
    ,_gdi32_hmodule(NULL)
    ,_current_usage(0.0)
    ,_timer_ID(0)
    ,_running_time(0)
{
    _performance_counter.QuadPart = 0;
}

GpuUsage::~GpuUsage()
{
    uninitalizeAdapter();
}

bool GpuUsage::initializeAdapter()
{
    if(isXPorEarlier()) return false; 
    if(_is_initialized) return true;

    LUID luid = {20};
    TOKEN_PRIVILEGES privs = {1, {luid, SE_PRIVILEGE_ENABLED}};
    HMODULE hDesktopModule = reinterpret_cast<HMODULE>(GetCurrentProcess());//GetModuleHandleA("dwm.exe");
    if(NULL == hDesktopModule) {
        PRINTF_DEBUG_MSG("initializeAdapter, failed to get module.");
        return false;
    }
    HANDLE hToken;
    BOOL bRet = OpenProcessToken(hDesktopModule, TOKEN_ADJUST_PRIVILEGES, &hToken);
    if(!bRet) {
        PRINTF_DEBUG_MSG("initializeAdapter, failed to open process token.");
        return false;
    }
    bRet = AdjustTokenPrivileges(hToken, FALSE, &privs, sizeof(privs), NULL, NULL);
    if(!bRet) {
        PRINTF_DEBUG_MSG("initializeAdapter, failed to adjust privileges.");
        return false;
    }
    D3DKMT_OPENADAPTERFROMDEVICENAME device_name = 
    {L"\\\\?\\PCI#VEN_8086&DEV_0102&SUBSYS_D0001458&REV_09#3&11583659&0&10#{1ca05180-a699-450a-9a0c-de4fbe3ddd89}"};
    _gdi32_hmodule = LoadLibraryA("gdi32.dll");
    if(!_gdi32_hmodule) {
        PRINTF_DEBUG_MSG("initializeAdapter, failed to load gdi32.dll");
        return false;
    }
    PFND3DKMT_OPENADAPTERFROMDEVICENAME D3DKMTOpenAdapterFromDeviceName =
        (PFND3DKMT_OPENADAPTERFROMDEVICENAME)GetProcAddress(_gdi32_hmodule, "D3DKMTOpenAdapterFromDeviceName");
    if(!D3DKMTOpenAdapterFromDeviceName) {
        FreeLibrary(_gdi32_hmodule);
        PRINTF_DEBUG_MSG("initializeAdapter, failed to get openAdapter address.");
        return false;
    }
    NTSTATUS nt_status = D3DKMTOpenAdapterFromDeviceName(&device_name);
    if(0>nt_status || 63<nt_status) {
        FreeLibrary(_gdi32_hmodule);
        PRINTF_DEBUG_MSG("initializeAdapter, failed to open adapter.");
        return false;
    }
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, TRUE, GetProcessId(hDesktopModule));
    if(!hProcess) {
        FreeLibrary(_gdi32_hmodule);
        PRINTF_DEBUG_MSG("initializeAdapter, failed to open process.");
        return false;
    }
    XD_D3DKMTQueryStatistics = (PFND3DKMT_QUERYSTATISTICS)GetProcAddress(_gdi32_hmodule, "D3DKMTQueryStatistics");
    if(!XD_D3DKMTQueryStatistics) {
        CloseHandle(hProcess);
        FreeLibrary(_gdi32_hmodule);
        PRINTF_DEBUG_MSG("initializeAdapter, failed to get queryStatistics address.");
        return false;
    }
    _query_statistics.Type = D3DKMT_QUERYSTATISTICS_NODE;
    _query_statistics.AdapterLuid = device_name.AdapterLuid;
    _query_statistics.hProcess = hProcess;
    _query_statistics.QueryNode.NodeId = 0;
    _is_initialized = true;
    startMonitor();
    return true;
}

void GpuUsage::uninitalizeAdapter()
{
    endMonitor();
    CloseHandle(_query_statistics.hProcess);
    FreeLibrary(_gdi32_hmodule);
    _query_statistics.hProcess = NULL;
    _gdi32_hmodule = NULL;
    _is_initialized = false;
}

bool GpuUsage::isGpuIdle()
{
    AutoLock auto_lock(&_usage_lock);
    return (_is_initialized && _current_usage<kIdleThreshold) ? true : false;
}

void CALLBACK processUsageTimer(UINT id, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
    GpuUsage *this_class = reinterpret_cast<GpuUsage*>(dwUser);
    this_class->updateUsage();
}

bool GpuUsage::startMonitor()
{
    if(!_is_initialized) return false;

    _timer_ID = timeSetEvent(1000, 5,
        processUsageTimer, reinterpret_cast<DWORD_PTR>(this), TIME_PERIODIC);
    return true;
}

void GpuUsage::updateUsage()
{
    if(!XD_D3DKMTQueryStatistics) {
        PRINTF_DEBUG_MSG("D3DKMTQueryStatistics function address is null.");
        return;
    }

    LARGE_INTEGER clock_frequency = {0};
    QueryPerformanceFrequency(&clock_frequency);
    LARGE_INTEGER counter = {0};
    QueryPerformanceCounter(&counter);
    LONGLONG delta_counter = counter.QuadPart - _performance_counter.QuadPart;
    // 逝去的时间，单位为纳秒
    double elapsed_time = static_cast<double>(delta_counter*10000000) / clock_frequency.QuadPart;
    NTSTATUS status = XD_D3DKMTQueryStatistics(&_query_statistics);
    if(0>status || 63<status) return;
    LONGLONG running_time = _query_statistics.QueryResult.NodeInformation.GlobalInformation.RunningTime.QuadPart;
    LONGLONG delta_time = running_time - _running_time;
    if(0<_performance_counter.QuadPart && 0<_running_time) {
        float usage = 0.0f;
        if(0 < elapsed_time) {
            usage = static_cast<float>(static_cast<double>(delta_time) / elapsed_time);
        }
        _usage_lock.Lock();
        _current_usage = usage;
        _usage_lock.Unlock();
        checkBusyState(usage);
    }
    _performance_counter = counter;
    _running_time = running_time;
}

void GpuUsage::checkBusyState(const float current_gpu)
{
    if(current_gpu >= kBusyThreshold) {
        //TODO:做相关操作
    }
}

void GpuUsage::endMonitor()
{
    if(_timer_ID) {
        timeKillEvent(_timer_ID);
        _timer_ID = 0;
    }
}