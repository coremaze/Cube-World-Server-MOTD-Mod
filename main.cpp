#undef __STRICT_ANSI__
#define no_shenanigans __attribute__((noinline)) __declspec(dllexport)
#include <windows.h>
#include <fstream>
#include "callbacks.h"
#include "packets.h"

unsigned int base;

void SendChatMessage(SOCKET socket, wchar_t* message){
    unsigned int len = wcslen(message);

    int pid = 10;
    long long int entity_id = 0;

    int pkt_size = 4 + 8 + 4 + len*2;
    char buf[pkt_size] = {0};
    memcpy(buf, &pid, 4);
    memcpy(buf + 4, &entity_id, 8);
    memcpy(buf + 12, &len, 4);
    memcpy(buf + 16, (char*)message, len*2);

    AddPacket(socket, buf, pkt_size);
}



void MOTD(SOCKET socket){
    //Check if this person was already greeted
    EnterCriticalSection(&known_sockets_critical_section);
    for (SOCKET sock : knownSockets){
        if (sock == socket){
            LeaveCriticalSection(&known_sockets_critical_section);
            return;
        }
    }
    AddSocket(socket);
    LeaveCriticalSection(&known_sockets_critical_section);

    //read motd from file
    char * buf;
    std::streampos fsize;
    std::string fileName = "motd.txt";
    std::ifstream file(fileName, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open()){
        //File exists, read it
        fsize = file.tellg();
        buf = new char[fsize + 1];
        file.seekg(0, std::ios::beg);
        file.read(buf, fsize);
        buf[fsize] = 0;
        file.close();

    } else {
        //bail out
        return;
    }


    //turn motd into wide string
    wchar_t wbuf[(int)fsize + 1] = {0};
    for (int i = 0; i<((int)fsize+1); i++){
        wbuf[i] = buf[i];
    }
    delete buf;

    //Send message
    SendChatMessage(socket, wbuf);
}

void __stdcall no_shenanigans HandleReadyToSend(SOCKET socket){
    MOTD(socket);
    SendQueuedPackets(socket);
}

void __stdcall no_shenanigans HandlePlayerDisconnect(SOCKET socket){
    PurgeSocket(socket);
}


DWORD WINAPI no_shenanigans RegisterCallbacks(){

        RegisterCallback("RegisterReadyToSendCallback", HandleReadyToSend);
        RegisterCallback("RegisterPlayerDisconnectCallback", HandlePlayerDisconnect);

        return 0;
}

extern "C" no_shenanigans bool APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    base = (unsigned int)GetModuleHandle(NULL);
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:

            PacketsInit();

            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RegisterCallbacks, 0, 0, NULL);
            break;
    }
    return true;
}
