// Powered By Xa9
// Github: 
// https://github.com/xa9_top/

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <windows.h>
#include <chrono>
#include <thread>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <curl/curl.h>
#include <archive.h>
#include <archive_entry.h>
#include <bzlib.h>

#pragma warning(disable : 4996)

using namespace std;
namespace fs = filesystem;
using boost::property_tree::ptree;

// 声明全局变量
string HTTPReturn;
string zipError;

LPCWSTR str2LPCW(string str)
{
    size_t size = str.length();
    int wLen = MultiByteToWideChar(CP_UTF8,
        0,
        str.c_str(),
        -1,
        NULL,
        0);
    wchar_t* buffer = new wchar_t[wLen + 1];
    memset(buffer, 0, (wLen + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), size, (LPWSTR)buffer, wLen);
    return buffer;
}

string getExecutablePath() {
    wchar_t result[MAX_PATH];
    GetModuleFileNameW(NULL, result, MAX_PATH);
    wstring wstr(result);

    // 使用 std::wstring_convert 和 std::codecvt_utf8 将 wstring 转换为 string
    using convert_type = codecvt_utf8<wchar_t>;
    wstring_convert<convert_type, wchar_t> converter;

    // 使用 converter 将 wstring 转换为 string
    string str = converter.to_bytes(wstr);

    // 移除文件名部分
    size_t lastSlashPos = str.rfind('\\');
    if (lastSlashPos != string::npos) {
        str.erase(lastSlashPos, str.length() - lastSlashPos);
    }

    return str;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

pair<string, DWORD> executeCommand(const string& command) {
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;

    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
        return make_pair("", ::GetLastError());
    }

    if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
        return make_pair("", ::GetLastError());
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = g_hChildStd_OUT_Wr;
    si.hStdOutput = g_hChildStd_OUT_Wr;
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    wstring cmdLine = wstring(command.begin(), command.end());
    if (!CreateProcess(NULL,
        const_cast<wchar_t*>(cmdLine.c_str()),
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi)) {
        return make_pair("", ::GetLastError());
    }

    CloseHandle(g_hChildStd_OUT_Wr);

    string output;
    DWORD dwRead;
    CHAR chBuf[2048];
    BOOL bSuccess = FALSE;

    for (;;) {
        bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, 2048, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) break;

        string chunk(chBuf, dwRead);
        output += chunk;
    }

    DWORD exitCode;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        exitCode = ::GetLastError();
    }

    CloseHandle(g_hChildStd_OUT_Rd);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return make_pair(output, exitCode);
}

void showMessage(const string& message, UINT type = MB_OK) {
    MessageBox(NULL, str2LPCW(message), TEXT("提示"), type);
}

void readIniFile(const string& filePath, ptree& pt) {
    try {
        boost::property_tree::ini_parser::read_ini(filePath, pt);
    }
    catch (const exception& e) {
        showMessage("ini配置文件格式有误!\n请检查UTCloud.ini配置文件后再运行本程序!\n若忘记格式，请删除ini文件再运行本程序，程序会为您自动创建。");
        exit(1);
    }
}

string currentDateTime() {
    auto now = chrono::system_clock::now();
    auto in_time_t = chrono::system_clock::to_time_t(now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", localtime(&in_time_t));
    return string(buffer);
}

bool downloadFile(const string& url, const string& outputPath) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            HTTPReturn = "无法连接至服务器: " + string(curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            return false;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);

        if (http_code == 200) {
            ofstream outFile(outputPath, ios::binary);
            if (!outFile) {
                HTTPReturn = "无法写入文件: " + outputPath;
                return false;
            }
            outFile << readBuffer;
            outFile.close();
            return true;
        }
        else {
            HTTPReturn = "HTTP 状态码: " + to_string(http_code) + "\nHTTP返回数据: " + readBuffer;
            return false;
        }
    }
    else {
        HTTPReturn = "无法初始化 libcurl";
        return false;
    }
}

bool unzipFile(const string& zipPath, const string& outputPath) {
    if (executeCommand("unzip " + zipPath + " -d " + outputPath).second == 0) return true; else return false;
}

bool zipFolder(const string& folderPath, const string& zipPath) {
    if (executeCommand("zip -q -r -j " + zipPath + " " + folderPath).second == 0) return true; else return false;
    return true;
}

bool uploadFile(const string& url, const string& filePath) {
    CURL* curl;
    CURLcode res;
    string readBuffer;
    struct curl_httppost* formpost = NULL;
    struct curl_httppost* lastptr = NULL;

    // 初始化 CURL
    curl = curl_easy_init();
    if (curl) {
        // 添加文件表单项
        curl_formadd(&formpost,
            &lastptr,
            CURLFORM_COPYNAME, "file",
            CURLFORM_FILE, filePath.c_str(),
            CURLFORM_END);

        // 设置 URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // 设置表单数据
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        // 设置写回调函数
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // 执行请求
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            HTTPReturn = "无法连接至服务器: " + string(curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            curl_formfree(formpost);
            return false;
        }

        // 获取 HTTP 状态码
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);
        curl_formfree(formpost);

        if (http_code == 200) {
            return true;
        }
        else {
            HTTPReturn = "HTTP 状态码: " + to_string(http_code) + "\nHTTP返回数据: " + readBuffer;
            return false;
        }
    }
    else {
        HTTPReturn = "无法初始化libcurl";
        return false;
    }
}

void createIniFile(const string& filePath) {
    ofstream iniFile(filePath);
    iniFile << "[UTCloud]\n"
        << ";云存档模式下载api\n"
        << "downloadapi = http://\n"
        << ";云存档模式上传api\n"
        << "uploadapi = http://\n"
        << ";本地模式存档路径\n"
        << "path = ./UTCloud/UNDERTALE\n"
        << ";存档备份文件夹路径\n"
        << "bak = ./UTCloud/bak\n"
        << ";模式，cloud表示云存档模式，local表示本地模式\n"
        << "mode = cloud\n"
        << ";状态，on表示开启存档同步，off表示不开启，如果为off则直接跳过有关存档的操作直接启动UT\n"
        << "state = on\n";
    iniFile.close();
}

void startUndertale() {
    system("start /wait UNDERTALE.exe");
}

void handleCloudMode(const ptree& pt) {
    string downloadapi = pt.get<string>("UTCloud.downloadapi");
    string uploadapi = pt.get<string>("UTCloud.uploadapi");
    string bakPath = pt.get<string>("UTCloud.bak");
    string localAppData = getenv("LOCALAPPDATA");
    string undertalePath = localAppData + "/UNDERTALE";
    string bakDir = bakPath + string("/") + currentDateTime();

    if (!fs::exists(undertalePath)) {
        startUndertale();
        // 上传文件
        zipFolder(undertalePath, "UNDERTALE.zip");
        if (!uploadFile(uploadapi, (getExecutablePath() + "\\UNDERTALE.zip"))) {
            showMessage("上传失败：" + HTTPReturn);
            exit(1);
        }
    }

    if (!fs::exists(bakPath)) {
        fs::create_directories(bakPath);
    }

    fs::create_directories(bakDir);
    fs::copy(undertalePath, bakDir, fs::copy_options::recursive);

    try {
        // 使用remove_all函数删除目录
        uintmax_t numRemoved = fs::remove_all(undertalePath);
    }
    catch (fs::filesystem_error& e) {
        string err = e.what();
        showMessage("无法删除文件：" + err);
    }

    printf((getExecutablePath() + "\\UNDERTALE.zip").c_str());

    // 下载 zip 文件并解压缩
    if (!downloadFile(downloadapi, (getExecutablePath() + "\\UNDERTALE.zip"))) {
        showMessage("下载失败：" + HTTPReturn);
        exit(1);
    }

    printf("\nsucc");

    try {
        // 使用create_directories函数创建目录
        bool created = fs::create_directories(undertalePath);
    }
    catch (fs::filesystem_error& e) {
        string err = e.what();
        showMessage("无法删除文件：" + err);
    }

    this_thread::sleep_for(chrono::milliseconds(500));

    unzipFile("UNDERTALE.zip", undertalePath);

    this_thread::sleep_for(chrono::milliseconds(200));
    startUndertale();

    // 上传文件
    zipFolder(undertalePath, "UNDERTALE.zip");
    if (!uploadFile(uploadapi, (getExecutablePath() + "\\UNDERTALE.zip"))) {
        showMessage("上传失败：" + HTTPReturn);
        exit(1);
    }
}

void handleLocalMode(const ptree& pt) {
    string path = pt.get<string>("UTCloud.path");
    string bakPath = pt.get<string>("UTCloud.bak");
    string localAppData = getenv("LOCALAPPDATA");
    string undertalePath = localAppData + "/UNDERTALE";
    string bakDir = (bakPath + "/" + currentDateTime());

    if (!fs::exists(undertalePath)) {
        startUndertale();
        return;
    }

    if (!fs::exists(bakPath)) {
        fs::create_directories(bakPath);
    }

    fs::create_directories(bakDir);
    fs::copy(undertalePath, bakDir, fs::copy_options::recursive);

    try {
        // 使用remove_all函数删除目录
        uintmax_t numRemoved = fs::remove_all(undertalePath);
    }
    catch (fs::filesystem_error& e) {
        string err = e.what();
        showMessage("无法删除文件：" + err);
    }

    if (!fs::exists(path)) {
        fs::create_directories(path);
    }

    fs::copy(path, undertalePath, fs::copy_options::recursive);

    this_thread::sleep_for(chrono::milliseconds(200));
    startUndertale();

    fs::copy(undertalePath, path, fs::copy_options::recursive);
}

int main(int argc, char* argv[]) {
    //HWND hwnd = GetForegroundWindow();
    //ShowWindow(hwnd, SW_HIDE);

    ptree pt;
    string iniFilePath = "UTCloud.ini";

    if (!fs::exists(iniFilePath)) {
        createIniFile(iniFilePath);
        showMessage("ini配置文件不存在，已为您创建!\n请编辑UTCloud.ini配置文件再运行本程序。");
        return 0;
    }

    readIniFile(iniFilePath, pt);

    string state = pt.get<string>("UTCloud.state");
    if (state == "off") {
        startUndertale();
        return 0;
    }

    string mode = pt.get<string>("UTCloud.mode");
    if (mode == "cloud") {
        handleCloudMode(pt);
    }
    else if (mode == "local") {
        handleLocalMode(pt);
    }

    return 0;
}