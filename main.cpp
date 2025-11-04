#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <set>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <powrprof.h>
#pragma comment(lib, "PowrProf.lib")
#endif
#include "rbx.hpp"
#include "offsets.hpp"

std::atomic<bool> keep_running{ true };
std::set<std::string> notifiedPlayers;

void signal_handler(int) {
    keep_running = false;
}

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::set<std::string> loadTargetPlayers(const std::string& filename) {
    std::set<std::string> players;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Couldn't find " << filename << ". Make sure it's in the same folder as this program.\n";
        return players;
    }

    std::string line;
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (!line.empty() && line[0] != '#') {
            players.insert(line);
        }
    }

    file.close();

    if (players.empty()) {
        std::cerr << "No players found in " << filename << ". Add some usernames!\n";
    }

    return players;
}

#ifdef _WIN32
void sendNotification(const std::string& playerName) {

    if (notifiedPlayers.count(to_lower(playerName)) > 0) {
        return;
    }

    notifiedPlayers.insert(to_lower(playerName));

    std::wstring title = L"Moderator Detected";
    std::wstring message = L"Moderator " + std::wstring(playerName.begin(), playerName.end()) + L" has joined the game!";
    std::wstring fullMessage = title + L"\n\n" + message;

    std::wstring psCommand =
        L"powershell -WindowStyle Hidden -Command \""
        L"[Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, ContentType = WindowsRuntime] > $null;"
        L"$template = [Windows.UI.Notifications.ToastNotificationManager]::GetTemplateContent([Windows.UI.Notifications.ToastTemplateType]::ToastText02);"
        L"$toastXml = [xml] $template.GetXml();"
        L"$toastXml.GetElementsByTagName('text')[0].AppendChild($toastXml.CreateTextNode('" + title + L"')) > $null;"
        L"$toastXml.GetElementsByTagName('text')[1].AppendChild($toastXml.CreateTextNode('" + message + L"')) > $null;"
        L"$xml = New-Object Windows.Data.Xml.Dom.XmlDocument;"
        L"$xml.LoadXml($toastXml.OuterXml);"
        L"$toast = [Windows.UI.Notifications.ToastNotification]::new($xml);"
        L"$toast.ExpirationTime = [DateTimeOffset]::Now.AddSeconds(5);"
        L"[Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier('Perousia Softworks').Show($toast);"
        L"\"";

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    wchar_t* cmd = _wcsdup(psCommand.c_str());
    if (CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    free(cmd);

    std::cout << "Sent notification for: " << playerName << "\n";
}
#else
void sendNotification(const std::string& playerName) {
    std::cout << "Moderator " << playerName << " has joined\n";
}
#endif

int main() {
    std::signal(SIGINT, signal_handler);
#ifdef _WIN32
    std::signal(SIGTERM, signal_handler);
#endif

    if (!RBX::Memory::attach()) {
        std::cerr << "Couldn't connect to Roblox.\n";
        std::cerr << "Make sure Roblox is running and run this as Administrator!\n";
        std::cerr << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }

    void* baseAddr = RBX::Memory::getRobloxBaseAddr();
    if (baseAddr == nullptr) {
        std::cerr << "Failed to get Roblox base address!\n";
        std::cerr << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }

    std::set<std::string> targetPlayers = loadTargetPlayers("playerlist.txt");
    if (targetPlayers.empty()) {
        std::cerr << "\nPress Enter to exit...";
        std::cin.get();
        return 1;
    }

    std::set<std::string> targetPlayersLower;
    for (const auto& t : targetPlayers) targetPlayersLower.insert(to_lower(t));

    std::set<std::string> prevPlayers;
    bool firstRun = true;
    int failCount = 0;

    while (keep_running) {
#ifdef _WIN32
        system("cls");
#else
        std::cout << "\033[2J\033[H";
#endif

        RBX::Instance dataModel{ RBX::getDataModel() };
        if (dataModel.address == 0 || dataModel.address == nullptr) {
            failCount++;
            if (failCount > 5) {
                std::cerr << "\nFailed to get DataModel. Your offsets might be outdated!\n";
                std::cerr << "\nPress Enter to exit...";
                std::cin.get();
                return 1;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        failCount = 0;

        RBX::Instance players{ dataModel.findFirstChild("Players") };
        if (players.address == 0 || players.address == nullptr) {
            std::cout << "Unable to find playerservice\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        std::set<std::string> currentPlayers;
        auto children = players.getChildren();
        for (RBX::Instance plr : children) {
            if (plr.address == 0) continue;
            std::string playerName = plr.name();
            currentPlayers.insert(playerName);
            std::cout << playerName << '\n';
        }

        std::set<std::string> currentPlayersLower;
        for (const auto& name : currentPlayers) {
            currentPlayersLower.insert(to_lower(name));
        }

        if (prevPlayers.empty() && !currentPlayers.empty()) {
            firstRun = true;
            notifiedPlayers.clear();
        }

        if (firstRun) {
            for (const auto& target : targetPlayersLower) {
                if (currentPlayersLower.count(target)) {
                    std::string realName;
                    for (const auto& p : currentPlayers)
                        if (to_lower(p) == target) { realName = p; break; }
                    std::cout << "Moderator " << realName << " is already in the game\n";
                    sendNotification(realName);
                }
            }
            firstRun = false;
        }
        else {
            for (const auto& target : targetPlayersLower) {
                if (currentPlayersLower.count(target)) {
                    bool wasInPrevious = false;
                    for (const auto& prev : prevPlayers) {
                        if (to_lower(prev) == target) {
                            wasInPrevious = true;
                            break;
                        }
                    }

                    if (!wasInPrevious) {
                        std::string realName;
                        for (const auto& p : currentPlayers)
                            if (to_lower(p) == target) { realName = p; break; }
                        std::cout << "MODERATOR JOINED: " << realName << "\n";
                        sendNotification(realName);
                    }
                }
            }
        }

        prevPlayers = currentPlayers;
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    RBX::Memory::detach();
    return 0;
}
