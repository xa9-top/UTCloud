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

// ����ȫ�ֱ���
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

    // ʹ�� std::wstring_convert �� std::codecvt_utf8 �� wstring ת��Ϊ string
    using convert_type = codecvt_utf8<wchar_t>;
    wstring_convert<convert_type, wchar_t> converter;

    // ʹ�� converter �� wstring ת��Ϊ string
    string str = converter.to_bytes(wstr);

    // �Ƴ��ļ�������
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
    MessageBox(NULL, str2LPCW(message), TEXT("��ʾ"), type);
}

void readIniFile(const string& filePath, ptree& pt) {
    try {
        boost::property_tree::ini_parser::read_ini(filePath, pt);
    }
    catch (const exception& e) {
        showMessage("ini�����ļ���ʽ����!\n����UTCloud.ini�����ļ��������б�����!\n�����Ǹ�ʽ����ɾ��ini�ļ������б����򣬳����Ϊ���Զ�������");
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
            HTTPReturn = "�޷�������������: " + string(curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            return false;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);

        if (http_code == 200) {
            ofstream outFile(outputPath, ios::binary);
            if (!outFile) {
                HTTPReturn = "�޷�д���ļ�: " + outputPath;
                return false;
            }
            outFile << readBuffer;
            outFile.close();
            return true;
        }
        else {
            HTTPReturn = "HTTP ״̬��: " + to_string(http_code) + "\nHTTP��������: " + readBuffer;
            return false;
        }
    }
    else {
        HTTPReturn = "�޷���ʼ�� libcurl";
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

    // ��ʼ�� CURL
    curl = curl_easy_init();
    if (curl) {
        // ����ļ�����
        curl_formadd(&formpost,
            &lastptr,
            CURLFORM_COPYNAME, "file",
            CURLFORM_FILE, filePath.c_str(),
            CURLFORM_END);

        // ���� URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // ���ñ�����
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        // ����д�ص�����
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // ִ������
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            HTTPReturn = "�޷�������������: " + string(curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            curl_formfree(formpost);
            return false;
        }

        // ��ȡ HTTP ״̬��
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_easy_cleanup(curl);
        curl_formfree(formpost);

        if (http_code == 200) {
            return true;
        }
        else {
            HTTPReturn = "HTTP ״̬��: " + to_string(http_code) + "\nHTTP��������: " + readBuffer;
            return false;
        }
    }
    else {
        HTTPReturn = "�޷���ʼ��libcurl";
        return false;
    }
}

void createIniFile(const string& filePath) {
    ofstream iniFile(filePath);
    iniFile << "[UTCloud]\n"
        << ";�ƴ浵ģʽ����api\n"
        << "downloadapi = http://\n"
        << ";�ƴ浵ģʽ�ϴ�api\n"
        << "uploadapi = http://\n"
        << ";����ģʽ�浵·��\n"
        << "path = ./UTCloud/UNDERTALE\n"
        << ";�浵�����ļ���·��\n"
        << "bak = ./UTCloud/bak\n"
        << ";ģʽ��cloud��ʾ�ƴ浵ģʽ��local��ʾ����ģʽ\n"
        << "mode = cloud\n"
        << ";״̬��on��ʾ�����浵ͬ����off��ʾ�����������Ϊoff��ֱ�������йش浵�Ĳ���ֱ������UT\n"
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
        // �ϴ��ļ�
        zipFolder(undertalePath, "UNDERTALE.zip");
        if (!uploadFile(uploadapi, (getExecutablePath() + "\\UNDERTALE.zip"))) {
            showMessage("�ϴ�ʧ�ܣ�" + HTTPReturn);
            exit(1);
        }
    }

    if (!fs::exists(bakPath)) {
        fs::create_directories(bakPath);
    }

    fs::create_directories(bakDir);
    fs::copy(undertalePath, bakDir, fs::copy_options::recursive);

    try {
        // ʹ��remove_all����ɾ��Ŀ¼
        uintmax_t numRemoved = fs::remove_all(undertalePath);
    }
    catch (fs::filesystem_error& e) {
        string err = e.what();
        showMessage("�޷�ɾ���ļ���" + err);
    }

    printf((getExecutablePath() + "\\UNDERTALE.zip").c_str());

    // ���� zip �ļ�����ѹ��
    if (!downloadFile(downloadapi, (getExecutablePath() + "\\UNDERTALE.zip"))) {
        showMessage("����ʧ�ܣ�" + HTTPReturn);
        exit(1);
    }

    printf("\nsucc");

    try {
        // ʹ��create_directories��������Ŀ¼
        bool created = fs::create_directories(undertalePath);
    }
    catch (fs::filesystem_error& e) {
        string err = e.what();
        showMessage("�޷�ɾ���ļ���" + err);
    }

    this_thread::sleep_for(chrono::milliseconds(500));

    unzipFile("UNDERTALE.zip", undertalePath);

    this_thread::sleep_for(chrono::milliseconds(200));
    startUndertale();

    // �ϴ��ļ�
    zipFolder(undertalePath, "UNDERTALE.zip");
    if (!uploadFile(uploadapi, (getExecutablePath() + "\\UNDERTALE.zip"))) {
        showMessage("�ϴ�ʧ�ܣ�" + HTTPReturn);
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
        // ʹ��remove_all����ɾ��Ŀ¼
        uintmax_t numRemoved = fs::remove_all(undertalePath);
    }
    catch (fs::filesystem_error& e) {
        string err = e.what();
        showMessage("�޷�ɾ���ļ���" + err);
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
        showMessage("ini�����ļ������ڣ���Ϊ������!\n��༭UTCloud.ini�����ļ������б�����");
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