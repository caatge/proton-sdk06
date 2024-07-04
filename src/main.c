#include "stdint.h"
#include "stdio.h"
#ifndef UNICODE
#define UNICODE
#endif // !UNICODE
#include "MinHook.h"
#include "windows.h"


//steamclient_call = (steamclient_call_t)((size_t)lsteamclient + 0xB990);
//size_t tohook = (size_t)lsteamclient + 0xEF400;

static int new_vstdlib = 0;

typedef void* (*CreateInterfaceFn)(const char* name, int*);

void (*Msg)(const char* fmt, ...);

typedef uint64_t CGameID;
typedef uint64_t CSteamID;

typedef int (*steamclient_call_t)(int a1, struct args_s*, const char* a3);
steamclient_call_t steamclient_call;
static int magic_number = 0;
const char* ifname = NULL;

int __fastcall winISteamUser_SteamUser005_InitiateGameConnection(
	void* steamusr,
	void* edx,
	void* pBlob,
	size_t cbMaxBlob,
	CSteamID steamID,
	CGameID nGameAppID,
	unsigned int unIPServer,
	uint32_t usPortServer,
	uint32_t secure)
{
#pragma pack(push, 1)
	struct args_s {
		size_t  steamuser_something;
		size_t ret;
		void*  pBlob;
		size_t cbMaxBlob;
		CSteamID steamID;
		CGameID* pnGameAppID;
		unsigned int unIPServer;
		uint32_t usPortServer;
		uint32_t secure;
	} args;
#pragma pack(pop)
	args.steamuser_something = ((size_t*)steamusr)[1];
	args.pBlob = pBlob;
	args.cbMaxBlob = cbMaxBlob;
	args.steamID = steamID;
	args.pnGameAppID = &nGameAppID;
	args.unIPServer = unIPServer;
	args.usPortServer = usPortServer;
	args.secure = secure; //5418 "ISteamUser_SteamUser005_InitiateGameConnection"
	steamclient_call(magic_number, &args, ifname);
	return args.ret;
}

DWORD __stdcall HookSteamAPIDelayed(LPVOID _) {

	static void* trampoline_not_used;

	typedef void* (*SteamUser_t)();

	HMODULE steam_api = 0;
	void* steamuser = 0;

	while (!steam_api) {
		steam_api = GetModuleHandle(L"steam_api.dll");
	}

	SteamUser_t SteamUser = (SteamUser_t)GetProcAddress(steam_api, "SteamUser");
	while (!steamuser) {
		steamuser = SteamUser();
	}

	Msg("SteamUser: %p\n", steamuser);

	void** vtable_SteamUser = *(void***)steamuser;

	Msg("SteamUser vtable: %p\n", vtable_SteamUser);

	Msg("InitiateGameConnection: %p\n", vtable_SteamUser[17]);


	// steamclient_call
	uint8_t* cur_byte = (uint8_t*)vtable_SteamUser[17];
	while (*cur_byte != 0xE8) {
		cur_byte++;
	};
	
	// resolve x86 relative near call
	size_t baseaddr = (size_t)(cur_byte + 4);
	size_t toadd = *(size_t*)(cur_byte + 1);
	size_t resolved = baseaddr + toadd;

	Msg("x86 call found at: %p, resolves to %p\n", cur_byte, resolved);

	// get magic
	while (cur_byte[0] != 0xC7 && cur_byte[1] != 0x04 && cur_byte[2] != 0x24) {
		cur_byte--;
	}

	magic_number = *(int*)(cur_byte + 3);
	Msg("magic: %i\n", magic_number);

	// get ifname
	while (cur_byte[0] != 0xC7 && cur_byte[1] != 0x44 && cur_byte[2] != 0x24) {
		cur_byte--;
	}

	ifname = *(const char**)(cur_byte + 4);
	Msg("ifname: %s\n", ifname);

	steamclient_call = (steamclient_call_t)resolved;

	MH_CreateHook(vtable_SteamUser[17], &winISteamUser_SteamUser005_InitiateGameConnection, &trampoline_not_used);
	MH_EnableHook(MH_ALL_HOOKS);
	return 1;
}

char __fastcall Load(void* this, void* edx, void* f1, void* f2) {
	MH_Initialize();
	void* vstdlib = GetModuleHandle(L"vstdlib.dll");
	void* tier0 = GetModuleHandle(L"tier0.dll");
	if (!vstdlib || !tier0) return 0;

	Msg = (void(*)(const char*, ...))GetProcAddress(tier0, "Msg");
	CreateInterfaceFn vstdlib_factory = (CreateInterfaceFn)GetProcAddress(vstdlib, "CreateInterface");

	char iname[128];
	int current_version = 1;
	void* found = NULL;

	for (;;) {

		sprintf_s(iname, sizeof(iname), "VEngineCvar%03d", current_version);

		void* _current = vstdlib_factory(iname, NULL);
		if (found && !_current) {
			current_version--;
			break;
		}
		current_version++;
		found = _current;
	}

	if (current_version > 3) new_vstdlib = 1;

	CreateThread(0, 0, &HookSteamAPIDelayed, 0, 0, 0);

	Msg("Auth fix plugin loaded!\n");

	return 1;
}

// VSP stub below (nothing interesting)
void Unload() {
	MH_Uninitialize();
	return;
}

void Pause() {}
void UnPause() {}

const char* GetPluginDescription() {
	return "Auth fix plugin by @gmod9";
}

void __stdcall LevelInit(char const* pMapName) {}
void __stdcall ServerActivate(void* pEdictList, int edictCount, int clientMax) {}
void __stdcall GameFrame(char simulating) {}
void __stdcall LevelShutdown(void) {}
void __stdcall ClientActive(void* pEntity) {}
void __stdcall ClientDisconnect(void* pEntity) {}
void __stdcall ClientPutInServer(void* pEntity, char const* playername) {}
void __stdcall SetCommandClient(int index) {}
void __stdcall ClientSettingsChanged(void* pEdict) {}

int __stdcall ClientConnect(void* bAllowConnect, void* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen) { return 0; }

void ClientCommand() {
	__asm {
		mov eax, [new_vstdlib]

		cmp eax, 0
		je old_interface
		xor eax, eax
		ret 8

		old_interface :
		xor eax, eax
		ret 4
	}
}

int __stdcall  NetworkIDValidated(const char* pszUserName, const char* pszNetworkID) { return 0; };
void __stdcall OnQueryCvarValueFinished(int iCookie, void* pPlayerEntity, int eStatus, const char* pCvarName, const char* pCvarValue) {};
void __stdcall OnEdictAllocated(void* edict) {};
void __stdcall OnEdictFreed(const void* edict) {};

static void* vtable[] = {
	Load,
	Unload,
	Pause,
	UnPause,
	GetPluginDescription,
	LevelInit,
	ServerActivate,
	GameFrame,
	LevelShutdown,
	ClientActive,
	ClientDisconnect,
	ClientPutInServer,
	SetCommandClient,
	ClientSettingsChanged,
	ClientConnect,
	ClientCommand,
	NetworkIDValidated,
	OnQueryCvarValueFinished,
	OnEdictAllocated,
	OnEdictFreed
};

void* pVtable = &vtable;
void* ppVtable = &pVtable;

__declspec(dllexport) void* CreateInterface(const char* name, int* _) {
	if (strstr(name, "ISERVERPLUGINCALLBACKS")) return ppVtable;
	else return NULL;
}