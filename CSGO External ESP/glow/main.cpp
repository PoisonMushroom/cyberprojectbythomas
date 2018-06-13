	#include "stdafx.h"
    #include <windows.h>
    #include <iostream>
	#include "Memory.h"
	#include "vector.h"
     
	//Integers will be unsigned
    typedef unsigned char uint8_t;
     
	//Catches invalid types during compile time
    template <typename T, size_t N>
     
    size_t countof(T(&array)[N])
    {
    	return N;
    }
     
	//OFFSETS
    DWORD dwLocalPlayer;
    DWORD dwEntityList;
    DWORD dwGlow;
	DWORD dwJump;
	DWORD dwForceAttack;
	DWORD dwClientState_ViewAngles;
	DWORD dwClientState;
	DWORD dwClientState_state;
     
	//READONLY OFFSETS
    DWORD dwTeam = 0xF0;
    DWORD dwDormant = 0xE9;
	DWORD dwFlags = 0x100;
	DWORD m_iShotsFired = 0xA2B0;
	DWORD m_aimNewAngle = 0x301C;
	DWORD m_iCrosshairId = 0xB2A4;
	DWORD m_lifeState = 0x25B;
	DWORD DwEntitySize = 0x10;

    //Glow variables
    struct glow_t
    {
    	DWORD dwBase;
    	float r;
    	float g;
    	float b;
    	float a;
    	uint8_t unk1[16];
    	bool m_bRenderWhenOccluded;
    	bool m_bRenderWhenUnoccluded;
    	bool m_bFullBloom;
    	uint8_t unk2[14];
    };
     
    //Entities
    struct Entity
    {
    	DWORD dwBase;
    	int team;
    	bool is_dormant;
    };
     
    //Player information
    struct Player
    {
    	DWORD dwBase;
    	bool isDormant;
    };
     
	//Allocates system and memory information
    process memory;
    process _modClient;
    process* mem;
    PModule modClient;
	PModule modEngine;
     
	//Used to count the number of friendlies and enemies in a server
    int friendlies;
    int enemies;
     
    Entity entEnemies[32];
    Entity entFriendlies[32];
    Entity me;
     
    void update_entity_data(Entity* e, DWORD dwBase)
    {
    	int dormant = memory.Read<int>(dwBase + dwDormant);
    	e->dwBase = dwBase;
    	e->team = memory.Read<int>(dwBase + dwTeam);
    	e->is_dormant = dormant == 1;
    }
    //Pointer information
    PModule* GetClientModule() {
    	if (modClient.dwBase == 0 && modClient.dwSize == 0) {
    		modClient = memory.GetModule("client.dll");
    	}
    	return &modClient;
    }

	PModule* GetEngineModule() {
		if (modEngine.dwBase == 0 && modEngine.dwSize == 0) {
			modEngine = memory.GetModule("engine.dll");
		}
		return &modEngine;
	}
     
	//FRIENDLY AND ENEMY INFORMATION
    Entity* GetEntityByBase(DWORD dwBase) {
     
    	for (int i = 0; i < friendlies; i++) {
    		if (dwBase == entFriendlies[i].dwBase) {
    			return &entFriendlies[i];
    		}
    	}
    	for (int i = 0; i < enemies; i++) {
    		if (dwBase == entEnemies[i].dwBase) {
    			return &entEnemies[i];
    		}
    	}
    	return nullptr;
    }
     
    //https://www.mpgh.net/forum/showthread.php?t=84823
	//AUTOMATIC OFFSET DUMPER
    class offset
    {
    private:
    	static void update_local_player() {
    		DWORD lpStart = mem->FindPatternArray(modClient.dwBase, modClient.dwSize, "xxx????xx????xxxxx?", 19, 0x8D, 0x34, 0x85, 0x0, 0x0, 0x0, 0x0, 0x89, 0x15, 0x0, 0x0, 0x0, 0x0, 0x8B, 0x41, 0x8, 0x8B, 0x48, 0x0);
    		DWORD lpP1 = mem->Read<DWORD>(lpStart + 3);
    		BYTE lpP2 = mem->Read<BYTE>(lpStart + 18);
    		dwLocalPlayer = (lpP1 + lpP2) - modClient.dwBase;
    	}
     
    	static void update_entity_list() {
    		DWORD elStart = mem->FindPatternArray(modClient.dwBase, modClient.dwSize, "x????xx?xxx", 11, 0x5, 0x0, 0x0, 0x0, 0x0, 0xC1, 0xE9, 0x0, 0x39, 0x48, 0x4);
    		DWORD elP1 = mem->Read<DWORD>(elStart + 1);
    		BYTE elP2 = mem->Read<BYTE>(elStart + 7);
    		dwEntityList = (elP1 + elP2) - modClient.dwBase;
    	}
     
    	static void update_glow() {
    		DWORD gpStart = mem->FindPatternArray(modClient.dwBase, modClient.dwSize, "xxx????xxxxx????????", 20, 0x0F, 0x11, 0x05, 0x0, 0x0, 0x0, 0x0, 0x83, 0xC8, 0x01, 0xC7, 0x05, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
    		dwGlow = mem->Read<DWORD>(gpStart + 3) - modClient.dwBase;
    	}

		static void update_Jump() {
			DWORD jStart = mem->FindPatternArray(modClient.dwBase, modClient.dwSize, "xx????xxxxxxx", 23, 0x8B, 0x0D, 0x0, 0x0, 0x0, 0x0, 0x8B, 0xD6, 0x8B, 0xC1, 0x83, 0xCA, 0x02);
			DWORD jOff = mem->Read<DWORD>(jStart + 2);
			dwJump = jOff - modClient.dwBase;
		}

		static void update_ViewAngles() {
			DWORD esStart = mem->FindPatternArray(modEngine.dwBase, modEngine.dwSize, "xxxx????xxxxx", 13, 0xF3, 0x0F, 0x11, 0x80, 0x0, 0x0, 0x0, 0x0, 0xD9, 0x46, 0x04, 0xD9, 0x05);
			dwClientState_ViewAngles = mem->Read<DWORD>(esStart + 4);
		}

		static void update_ClientState() {
			DWORD epStart = mem->FindPatternArray(modEngine.dwBase, modEngine.dwSize, "x????xxx?x?xxxx", 15, 0xA1, 0x0, 0x0, 0x0, 0x0, 0x33, 0xD2, 0x6A, 0x0, 0x6A, 0x0, 0x33, 0xC9, 0x89, 0xB0);
			dwClientState = mem->Read<DWORD>(epStart + 1) - modEngine.dwBase;
		}
		
		static void update_ClientState_state() {
		DWORD ewpStart = mem->FindPatternArray(modEngine.dwBase, modEngine.dwSize, "xx????xxxx", 11, 0x83, 0xB8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x94, 0xC0, 0xC3);
		dwClientState_state = mem->Read<DWORD>(ewpStart + 2);
		}

    public:
		//Grabs local CSGO dynamic link library to dump offsets
    	static void get_offset(process* m) {
    		mem = m;
    		modClient = mem->GetModule("client.dll");
			modEngine = mem->GetModule("engine.dll");
			update_local_player();
    		update_entity_list();
    		update_glow();
			update_Jump();
			update_ViewAngles();
			update_ClientState();
			update_ClientState_state();
    	}
     
    	static DWORD WINAPI scan_offsets(LPVOID PARAM)
    	{
    		Entity players[64];
    		while (true) {
				Sleep(1);
    			DWORD playerBase = memory.Read<DWORD>(GetClientModule()->dwBase + dwLocalPlayer);
    			int cp = 0;
     
    			update_entity_data(&me, playerBase);
    			for (int i = 1; i < 64; i++) {
    				DWORD entBase = memory.Read<DWORD>((GetClientModule()->dwBase + dwEntityList) + i * 0x10);
     
    				if (entBase == NULL)
    					continue;
     
    				update_entity_data(&players[cp], entBase);
     
    				cp++;
    			}
     
    			int cf = 0, ce = 0;
     
    			for (int i = 0; i < cp; i++) {
    				if (players[i].team == me.team) {
    					entFriendlies[cf] = players[i];
    					cf++;
    				}
    				else {
    					entEnemies[ce] = players[i];
    					ce++;
    				}
    			}
    			enemies = ce;
    			friendlies = cf;
    		}
    	}
    };
    //END
     
    class virtualesp
    {
    private:
    	static void glow_player(DWORD mObj, float r, float g, float b)
    	{
    		memory.Write<float>(mObj + 0x4, r);
    		memory.Write<float>(mObj + 0x8, g);
    		memory.Write<float>(mObj + 0xC, b);
    		memory.Write<float>(mObj + 0x10, 1.0f);
    		memory.Write<BOOL>(mObj + 0x24, true);
    		memory.Write<BOOL>(mObj + 0x25, false);
    	}

    public:
    	static void start_engine() {
    		while (!memory.Attach("csgo.exe", PROCESS_ALL_ACCESS)) {
    			Sleep(100);
    		}
    		do {
    			Sleep(1000);
    			offset::get_offset(&memory);
    		} while (dwLocalPlayer < 65535);
    		CreateThread(NULL, NULL, &offset::scan_offsets, NULL, NULL, NULL);
    	}
     
    	static unsigned long __stdcall esp_thread(void*)
    	{
    		int objectCount;
    		DWORD pointerToGlow;
    		Entity* Player = NULL;
     
    		while (true)
    		{
				Sleep(10);
    			pointerToGlow = memory.Read<DWORD>(GetClientModule()->dwBase + dwGlow);
    			objectCount = memory.Read<DWORD>(GetClientModule()->dwBase + dwGlow + 0x4);
    			if (pointerToGlow != NULL && objectCount > 0)
    			{
    				for (int i = 0; i < objectCount; i++)
    				{
						//Sleep(1);

    					DWORD mObj = pointerToGlow + i * sizeof(glow_t);
    					glow_t glowObject = memory.Read<glow_t>(mObj);
    					Player = GetEntityByBase(glowObject.dwBase);
     
    					if (glowObject.dwBase == NULL || Player == nullptr || Player->is_dormant) {
    						continue;
    					}
    					if (me.team == Player->team) {
    						glow_player(mObj, 0, 255, 0); //FRIENDLY GLOW COLOR
    					}
    					else {
    						glow_player(mObj, 255, 0, 0); //ENEMY GLOW COLOR
    					}
    				}
    			}
    		}
    		return EXIT_SUCCESS;
    	}
    };

	//RETURNS OFFSETS
	DWORD GetEnginePointer()
	{
		return mem->Read<DWORD>(modEngine.dwBase + dwClientState);
	}

	bool IsInGame()
	{
		if (mem->Read<int>(GetEnginePointer() + dwClientState_state) == 6)
		{
			return true;
		}
		return false;
	}

	Vector GetViewAngles()
	{
		return mem->Read<Vector>(GetEnginePointer() + dwClientState_ViewAngles);
	}

	void SetViewAngles(Vector angles)
	{
		mem->Write<Vector>(GetEnginePointer() + dwClientState_ViewAngles, angles);
	}

	DWORD GetLocalPlayer()
	{
		return mem->Read<DWORD>(modClient.dwBase + dwLocalPlayer);
	}

	int GetShotsFired()
	{
		return mem->Read<int>(GetLocalPlayer() + m_iShotsFired);
	}

	int getLocalTeam()
	{
		return mem->Read<int>(GetLocalPlayer() + dwTeam);
	}

	int GetCrosshairId()
	{
		DWORD PlayerBase = GetLocalPlayer();
		if (PlayerBase)
		{
			return mem->Read<int>(PlayerBase + m_iCrosshairId) - 1;
		}
	}

	DWORD GetBaseEntity(int PlayerNumber)
	{
		return mem->Read<DWORD>(modClient.dwBase + dwEntityList + (DwEntitySize * PlayerNumber));
	}

	int GetTeam(int PlayerNumber)
	{
		DWORD BaseEntity = GetBaseEntity(PlayerNumber);
		if (BaseEntity)
		{
			return mem->Read<int>(BaseEntity + dwTeam);
		}
	}

	int GetTeam()
	{
		DWORD PlayerBase = GetLocalPlayer();
		if (PlayerBase)
		{
			return mem->Read<int>(PlayerBase + dwTeam);
		}
	}

	bool IsDead(int PlayerNumber)
	{
		DWORD BaseEntity = GetBaseEntity(PlayerNumber);
		if (BaseEntity)
		{
			return mem->Read<bool>(BaseEntity + m_lifeState);
		}
	}
	//END OF OFFSETS

	//HACKS
	class Cheats
	{
	public:

		//AUTOHOP
		static unsigned long __stdcall bhop_thread(void*)
		{
			while (true)
			{
				DWORD dwPla = memory.Read<DWORD>(GetClientModule()->dwBase + dwLocalPlayer);
				int flags = memory.Read<DWORD>(dwPla + dwFlags);

				if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && (flags & 0x1 == 1)) {
					memory.Write<int>(GetClientModule()->dwBase + dwJump, 5);
					Sleep(25);
					memory.Write<int>(GetClientModule()->dwBase + dwJump, 4);
				}
				Sleep(1);
			}
			return EXIT_SUCCESS;
		}

		//NO RECOIL
		static unsigned long __stdcall norecoil_thread(void*)
		{
			Vector viewAngle;
			Vector newAngle;
			Vector oldAngle;
			int rcsScale = 100;

			while (true)
			{
				if (IsInGame())
				{
					viewAngle = GetViewAngles();
					newAngle = mem->Read<Vector>(GetLocalPlayer() + m_aimNewAngle);

					//When firing bullets, grabs camera movement then subtracts itself
					if (GetShotsFired() > 1)
					{
						viewAngle.x -= (newAngle.x - oldAngle.x) * (rcsScale * 0.02f);
						viewAngle.y -= (newAngle.y - oldAngle.y) * (rcsScale * 0.02f);
						ClampVector(viewAngle);
						SetViewAngles(viewAngle);
						oldAngle.x = newAngle.x;
						oldAngle.y = newAngle.y;
					}
					else
					{
						oldAngle.x = newAngle.x;
						oldAngle.y = newAngle.y;
					}
				}
				Sleep(1);
			}
			return EXIT_SUCCESS;
		}

		//TRIGGER BOT
		static unsigned long __stdcall trigger_thread(void*)
		{
			while (true)
			{
				if (GetAsyncKeyState(VK_MENU) < 0 && VK_MENU != 1)
				{
					//GetCrosshairId becoming randomly null
					int PlayerNumber = GetCrosshairId();
					if (PlayerNumber < 64 && PlayerNumber >= 0 && GetTeam(PlayerNumber) != GetTeam() && IsDead(PlayerNumber) != true)
					{
						mouse_event(MOUSEEVENTF_LEFTDOWN, NULL, NULL, NULL, NULL);
						Sleep(200);
						mouse_event(MOUSEEVENTF_LEFTUP, NULL, NULL, NULL, NULL);
						Sleep(30);
					}
					else
					{
						Sleep(1);
					}
				}
				Sleep(1);
			}
			return EXIT_SUCCESS;
		}
	};
	 
    int main() 
    {
		//INITIALIZATION
    	bool glow = false;
		bool bhop = false;
		bool norecoil = false;
		bool trigger = false;

		//RESET ALL HACKS
    	HANDLE GLOW = NULL;
		HANDLE BHOP = NULL;
		HANDLE NORECOIL = NULL;
		HANDLE TRIGGER = NULL;

		//START HACK
    	virtualesp::start_engine();

		//CONSOLE MESSAGES
    	std::cout << "F1 to toggle Glow" << std::endl;
		std::cout << "F2 to toggle Bhop" << std::endl;
		std::cout << "F3 to toggle Norecoil" << std::endl;
		std::cout << "F4 to toggle Triggerbot" << std::endl;
		
		//OFFSET DUMPER INFORMATION
		std::cout << "" << std::endl;
		std::cout << "Offsets" << std::endl;
		std::cout << "" << std::endl;
		std::cout << "Localplayer = 0x" << std::hex << dwLocalPlayer << std::endl;
		std::cout << "Entitylist = 0x" << std::hex << dwEntityList << std::endl;
		std::cout << "Glowobject = 0x" << std::hex << dwGlow << std::endl;
		std::cout << "Forcejump = 0x" << std::hex << dwJump << std::endl;
		std::cout << "Viewangles = 0x" << std::hex << dwClientState_ViewAngles << std::endl;
		std::cout << "Clientstate = 0x" << std::hex << dwClientState << std::endl;
		std::cout << "Clientstate_state = 0x" << std::hex << dwClientState_state << std::endl;
		std::cout << "" << std::endl;

    	while (TRUE)
    	{
			//GLOW HACK
			Sleep(1);
    		if (GetAsyncKeyState(VK_F1) & 1) {
    			glow = !glow;
    			if (glow) {
    				std::cout << "GLOW ON" << std::endl;
    				GLOW = CreateThread(NULL, NULL, &virtualesp::esp_thread, NULL, NULL, NULL);
    			}
    			else {
    				std::cout << "GLOW OFF" << std::endl;
    				TerminateThread(GLOW, 0);
    				CloseHandle(GLOW);
    			}
    		}

			//AUTOHOP HACK
			if (GetAsyncKeyState(VK_F2) & 1) {
				bhop = !bhop;
				if (bhop) {
					std::cout << "AUTOHOP ON" << std::endl;
					BHOP = CreateThread(NULL, NULL, &Cheats::bhop_thread, NULL, NULL, NULL);
				}
				else {
					std::cout << "AUTOHOP OFF" << std::endl;
					TerminateThread(BHOP, 0);
					CloseHandle(BHOP);
				}
			}

			//NO RECOIL HACK
			if (GetAsyncKeyState(VK_F3) & 1) {
				norecoil = !norecoil;
				if (norecoil) {
					std::cout << "NORECOIL ON" << std::endl;
					NORECOIL = CreateThread(NULL, NULL, &Cheats::norecoil_thread, NULL, NULL, NULL);
				}
				else {
					std::cout << "NORECOIL OFF" << std::endl;
					TerminateThread(NORECOIL, 0);
					CloseHandle(NORECOIL);
				}
			}

			//TRIGGERBOT HACK
			if (GetAsyncKeyState(VK_F4) & 1) {
				trigger = !trigger;
				if (trigger) {
					std::cout << "TRIGGERBOT ON" << std::endl;
					TRIGGER = CreateThread(NULL, NULL, &Cheats::trigger_thread, NULL, NULL, NULL);
				}
				else {
					std::cout << "TRIGGERBOT OFF" << std::endl;
					TerminateThread(TRIGGER, 0);
					CloseHandle(TRIGGER);
				}
			}
    	}
    }
