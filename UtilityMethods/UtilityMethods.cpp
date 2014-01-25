#include "UtilityMethods.h"
#include "showery-safe-free.h"

//////////////////////////////////////////////////////////////////////////
///For both PlayerCore and MediaPlayer.

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

//将数据追加到到文件末尾。
void SaveDataToFile(const char *file_path, void *data, DWORD size)
{
    FILE *file_handle = nullptr;
    fopen_s(&file_handle, file_path, "ab+");
    if(nullptr!=file_handle) {
        if(0<size) {
            fseek(file_handle, 0, SEEK_END);
            fwrite(data, 1, size, file_handle);
        }
    }
    fclose(file_handle);
}

///================================================================
///Only for MediaPlayer.

//////////////////////////////////////////////////////////////////////////
///字符处理相关函数
//去掉mrl中file:\\\前缀，若没有此前缀则直接复制
//调用者需要释放返回值内存
char* mrlToFilePath(const char *mrl)
{
    if(nullptr == mrl) return nullptr;
    char *ret = nullptr;
    if(mrl ==strstr(mrl, "file:///")) {
        char *temp_mrl = _strdup(mrl);
        int path_len = strlen(mrl) - strlen("file:///");
        //TODO:
    }
    else if(mrl == strstr(mrl, "file:\\\\\\")){
        char *temp_mrl = _strdup(mrl);
        int path_len = strlen(mrl) - strlen("file:\\\\\\");
        char *path = temp_mrl + strlen("file:\\\\\\");
        ret = _strdup(path);
        SafeDeleteArray(&temp_mrl);
    }
    else {
        ret = _strdup(mrl);
    } 
    return ret;
}

//试图根据文件路径查找文件，以此判断文件路径是否存在
bool isFileExists(const char *path)
{
    WIN32_FIND_DATA find_file_data;
    HANDLE find_file = FindFirstFile(path, &find_file_data);
    if(INVALID_HANDLE_VALUE != find_file) {
        FindClose(find_file);
        return true;
    } else {
        return false;
    }
}

//多字节字符串转宽字符
//该函数为宽字符串的字符分配内存
//使用者需要释放返回值内存
wchar_t* charToWCHAR(const char *str)
{
    wchar_t *ret = nullptr;
    DWORD size = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
    ret = new wchar_t[size];
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, size);
    return ret;
}

//若字符串中包含网络路径造成的乱码，则修复之
std::string uriEncode(const std::string& s)
{
    char hex[] = "0123456789ABCDEF";
    std::string dst;

    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char cc = s[i];
        if (isascii(cc)) {
            if (cc == ' ') {
                dst += "%20";
            } else {
                dst += cc;
            }
        } else {
            unsigned char c = static_cast<unsigned char>(s[i]);
            dst += '%';
            dst += hex[c / 16];
            dst += hex[c % 16];
        }
    }
    return dst;
}

//如果是Windows平台，则将字符串转换为UTF-8编码
//否则直接返回
#if defined(WIN32) || defined(WIN64)
int ansiToUtf8(const char* ansi, char* utf8, size_t size)
{
    wchar_t tmp[1024]= {0};     
    int wl = MultiByteToWideChar(CP_ACP, 0, ansi, strlen(ansi), tmp, 1024); 
    int ml = WideCharToMultiByte(CP_UTF8, 0, tmp, wl, utf8, size, nullptr, nullptr);
    utf8[ml] = '\0';
    return ml;
}
#else
int ansiToUtf8(const char* ansi, char* utf8, size_t size)
{
    return 0;
}
#endif

//宽高比的浮点型值转换为X:Y的字符串形式
//调用者不需要也不可以释放返回值内存
const char *floatToAspectRatio(float val)
{
    if(val < 0.000001f)
        return "";
    if (val >0.0f && val < 1.24f)
        return "16:13";
    if (val > 1.24f && val < 1.30f)
        return "5:4";
    if (val > 1.30f && val < 1.40f)
        return "4:3";
    if (val > 1.45f && val < 1.55f)
        return "3:2";
    if (val > 1.55f && val < 1.65f)
        return "16:10";
    if (val > 1.65f && val < 1.95f)
        return "16:9";
    if (val > 1.95f && val < 2.05f)
        return "2:1";
    if (val > 2.05f && val < 2.25f)
        return "221:100";
    if (val > 2.25f && val < 2.30f)
        return "16:7";
    if (val > 2.30f && val < 2.37f)
        return "235:100";
    if (val > 2.37f && val < 2.40f)
        return "239:100";
    if (val > 2.45f && val < 2.55f)
        return "5:2";
    if (val > 2.90f && val < 3.10f)
        return "3:1";
    return "";
}

//////////////////////////////////////////////////////////////////////////
///DirectShow 连接Graph相关函数

//遍历base_filter所有Pin查找符合指定方向的Pin
//base_filter有且只有一个符合指定方向的Pin时调用该函数
HRESULT findOnlyPin(IBaseFilter *base_filter, IPin **ret_pin, const PIN_DIRECTION find_dir)
{
    if(nullptr==base_filter) {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    CComPtr<IEnumPins> enum_pins;
    hr = base_filter->EnumPins(&enum_pins);
    if(FAILED(hr)) {
        return hr;
    }
    CComPtr<IPin> temp_pin;
    enum_pins->Reset();
    while(SUCCEEDED(enum_pins->Next(1, &temp_pin, nullptr))) {
        //防止意外错误发生
        if(nullptr==temp_pin) break;
        PIN_DIRECTION pin_dir;
        temp_pin->QueryDirection(&pin_dir);
        if(find_dir != pin_dir) {
            temp_pin = nullptr;
            continue;
        }
        temp_pin.CopyTo(ret_pin);
        //This method needs to be called.
        //Because we are going to release temp_pin.
        //And the "=" operation doesn't increase reference number.
        temp_pin = nullptr;
        break;
    }

    if(nullptr==(*ret_pin))
        return E_FAIL;
    else
        return S_OK;
}

//调用findOnlyPin查找输出Pin
HRESULT findInputPin(IBaseFilter *base_filter, IPin **input_pin)
{
    return findOnlyPin(base_filter, input_pin, PINDIR_INPUT);
}

//调用findOnlyPin查找输出Pin
HRESULT findOutPin(IBaseFilter *base_filter, IPin **out_pin)
{
    return findOnlyPin(base_filter, out_pin, PINDIR_OUTPUT);
}

//遍历base_filter所有Pin查找其中符合指定名字和方向的Pin
HRESULT findPinByName(IBaseFilter *base_filter, IPin **ret_pin, const PIN_DIRECTION find_dir, LPCOLESTR pin_name)
{
    HRESULT hr = S_OK;
    CComPtr<IEnumPins> enum_pins;
    hr = base_filter->EnumPins(&enum_pins);
    if(FAILED(hr)) {
        return hr;
    }
    CComPtr<IPin> temp_pin;
    enum_pins->Reset();
    while(SUCCEEDED(enum_pins->Next(1, &temp_pin, nullptr))) {
        //防止意外错误发生
        if(nullptr==temp_pin) break;
        PIN_DIRECTION pin_dir;
        temp_pin->QueryDirection(&pin_dir);
        if(find_dir != pin_dir) {
            temp_pin = nullptr;
            continue;
        }
        PIN_INFO pin_info;
        temp_pin->QueryPinInfo(&pin_info);
        if(0==_wcsicmp(pin_name, pin_info.achName)) {
            SafeRelease(&pin_info.pFilter);
            temp_pin.CopyTo(ret_pin);
            break;
        }
        SafeRelease(&pin_info.pFilter);
        temp_pin = nullptr;
    }

    if(nullptr==(*ret_pin))
        return E_FAIL;
    else
        return S_OK;
}

//调用findPinByName查找输入Pin
HRESULT findInputPinByName(IBaseFilter *base_filter, IPin **input_pin, LPCOLESTR pin_name)
{
    return findPinByName(base_filter, input_pin, PINDIR_INPUT, pin_name);
}

//调用findPinByName查找输出Pin
HRESULT findOutPinByName(IBaseFilter *base_filter, IPin **out_pin, LPCOLESTR pin_name)
{
    return findPinByName(base_filter, out_pin, PINDIR_OUTPUT, pin_name);
}

//找出base_filter的第一个输入pin，将out_pin连接到该输入pin
HRESULT connectTwoFilters(IGraphBuilder *graph_builder, IPin *out_pin, IBaseFilter *base_filter)
{
    if(nullptr==graph_builder || nullptr==out_pin || nullptr==base_filter) return E_POINTER;

    HRESULT hr = S_OK;
    CComPtr<IPin> input_pin;
    hr = findInputPin(base_filter, &input_pin);
    if(FAILED(hr) || nullptr==input_pin) {
        return E_FAIL;
    }
    hr = graph_builder->Connect(out_pin, input_pin);
    return hr;
}

//找出first_filter的唯一输出pin和second_filter的第一个输入pin，连接两个pin
HRESULT connectTwoFilters(IGraphBuilder *graph_builder, IBaseFilter *first_filter, IBaseFilter *second_filter)
{
    if(nullptr==graph_builder || nullptr==first_filter || nullptr==second_filter) {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    CComPtr<IPin> input_pin, out_pin;
    hr = findOutPin(first_filter, &out_pin);
    if(FAILED(hr) || nullptr==out_pin) {
        return E_FAIL;
    }
    hr = findInputPin(second_filter, &input_pin);
    if(FAILED(hr) || nullptr==input_pin) {
        return E_FAIL;
    }
    hr = graph_builder->Connect(out_pin, input_pin);

    return hr;
}

//查找已连接的输入、输出Pin，断开它们的连接
void disconnectFilter(IBaseFilter *base_filter, IGraphBuilder *graph_builder)
{
    if(nullptr==base_filter || nullptr==graph_builder) return;

    HRESULT hr = S_OK;
    CComPtr<IEnumPins> enum_pins;
    hr = base_filter->EnumPins(&enum_pins);
    if(FAILED(hr)) return;
    enum_pins->Reset();
    CComPtr<IPin> cur_pin, connect_pin;
    while(SUCCEEDED(enum_pins->Next(1, &cur_pin, nullptr))) {
        // 防止意外错误发生
        if(nullptr == cur_pin) break;
        cur_pin->ConnectedTo(&connect_pin);
        if(connect_pin) {
            graph_builder->Disconnect(connect_pin);
            graph_builder->Disconnect(cur_pin);
        }
        cur_pin = nullptr;
        connect_pin = nullptr;
    }
    //从graph builder中清除该filter
    graph_builder->RemoveFilter(base_filter);

    return;
}

//查找base_filter上指定名称的pin，若该pin已连接，断开其连接
void disconnectPinByName(IBaseFilter *base_filter, LPCOLESTR pin_name)
{
    if(nullptr == base_filter) return;

    HRESULT hr = S_OK;
    CComPtr<IEnumPins> enum_pins;
    hr = base_filter->EnumPins(&enum_pins);
    if(FAILED(hr)) return;
    enum_pins->Reset();
    CComPtr<IPin> cur_pin, connect_pin;
    while(SUCCEEDED(enum_pins->Next(1, &cur_pin, nullptr))) {
        // 防止意外错误发生
        if(nullptr == cur_pin) break;
        // 判断该pin名称是否为指定名称
        PIN_INFO pin_info;
        memset(&pin_info, 0, sizeof(pin_info));
        cur_pin->QueryPinInfo(&pin_info);
        SafeRelease(&pin_info.pFilter);
        if(0!=_wcsicmp(pin_name, pin_info.achName)) {
            cur_pin = nullptr;
            connect_pin = nullptr;
            continue;
        }
        // 判断该pin是否已连接
        cur_pin->ConnectedTo(&connect_pin);
        if(connect_pin) {
            connect_pin->Disconnect();
            cur_pin->Disconnect();
        }
        cur_pin = nullptr;
        connect_pin = nullptr;
    }
}

///================================================================
///Only for PlayerCore.