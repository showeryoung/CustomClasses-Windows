//声明需要用到的函数
#pragma once
// For CComPtr
#include <atlbase.h>
#include <DShow.h>
#include <string>

//操作系统是否为XP或更早版本
bool isXPorEarlier();

//将数据保存到文件。
void SaveDataToFile(const char *file_path, void *data, DWORD size);

//mrl转文件路径
char* mrlToFilePath(const char *mrl);

//是否文件路径
bool isFileExists(const char *path);

//多字节转宽字符
wchar_t* charToWCHAR(const char *str);

//修正网络路径乱码
std::string uriEncode(const std::string& s);

//ANSI转UTF-8
int ansiToUtf8(const char* ansi, char* utf8, size_t size);

//浮点型宽高比转换为字符串形式
const char* floatToAspectRatio(float val);

//查找唯一的Pin
HRESULT findOnlyPin(IBaseFilter *base_filter, IPin **ret_pin, const PIN_DIRECTION find_dir);

//查找唯一的输入Pin
HRESULT findInputPin(IBaseFilter *base_filter, IPin **input_pin);

//查找唯一的输出Pin
HRESULT findOutPin(IBaseFilter *base_filter, IPin **out_pin);

//根据名字和方向查找Pin
HRESULT findPinByName(IBaseFilter *base_filter, IPin **ret_pin, const PIN_DIRECTION find_dir, LPCOLESTR pin_name);

//根据名字查找输入Pin
HRESULT findInputPinByName(IBaseFilter *base_filter, IPin **input_pin, LPCOLESTR pin_name);

//根据名字查找输出Pin
HRESULT findOutPinByName(IBaseFilter *base_filter, IPin **out_pin, LPCOLESTR pin_name);

//将输出pin连接到base_filter的输入pin
HRESULT connectTwoFilters(IGraphBuilder *graph_builder, IPin *out_pin, IBaseFilter *base_filter);

//连接两个filters，从第一个连向第二个
HRESULT connectTwoFilters(IGraphBuilder *graph_builder, IBaseFilter *first_filter, IBaseFilter *second_filter);

//断开base_filter的连接，并将其从graph builder中移除
void disconnectFilter(IBaseFilter *base_filter, IGraphBuilder *graph_builder);

//断开base_filter指定名称的pin
void disconnectPinByName(IBaseFilter *base_filter, LPCOLESTR pin_name);