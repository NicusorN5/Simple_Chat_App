#include "Server.hpp"
#include <Windows.h>
#include <exception>
#include "resource.h"
#include <thread>
#include "ConnectionClosedException.hpp"
#include <format>

#pragma comment(lib, "ws2_32.lib")

HWND AppWindow;
HINSTANCE AppInstance;

constexpr const char* const AppClassName = "ChatAppClass";

constexpr int Menu_NewChat = 300;
constexpr int Menu_ExitOption = 301;
constexpr int Menu_SetNickName = 302;
constexpr int Menu_NetworkingSettings = 303;
constexpr int Menu_About = 304;

char User_Nickname[30]{};

HWND textbox;
HWND chatbox;
HWND sendButton;

bool serverOn = true;

Server* server = nullptr;
Client* client = nullptr;

auto __stdcall NewChat_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
{
	switch(msg)
	{
		case WM_INITDIALOG:
			return 1;
		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			return 1;
		case WM_COMMAND:
		{
			if(wParam == IDOK)
			{
				char* username = new char[31];
				memset(username, 0, sizeof(char) * 31);
				EndDialog(hwnd, reinterpret_cast<INT_PTR>(username));
			}
			if(wParam == IDCANCEL)
				EndDialog(hwnd, IDCANCEL);
			return 1;
		}
		default:
			return 0;
	}
}

auto __stdcall AboutDialog_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
{
	switch(msg)
	{
		case WM_INITDIALOG:
			return 1;
		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			return 1;
		case WM_COMMAND:
		{
			if(wParam == IDOK)
			{
				EndDialog(hwnd, IDOK);
				return 1;
			}
			return 0;
		}
		default:
			return 0;
	}
}

auto CheckTextEmpty(const char* text) -> bool
{
	if(text == nullptr) return true;
	size_t len = strlen(text);
	if(len == 0) return true;

	for(size_t i = 0; i < len; i++)
	{
		if(text[i] != ' ') return false;
	}
	return true;
}

auto __stdcall SetNickName_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT
{
	switch(msg)
	{
		case WM_INITDIALOG:
		{
			HWND textBox = GetDlgItem(hwnd, IDC_EDIT1);
			SetWindowTextA(textBox, User_Nickname);
			return 1;
		}
		case WM_CLOSE:
			EndDialog(hwnd, 1);
			return 1;
		case WM_COMMAND:
		{
			if(wParam == IDOK)
			{
				HWND textBox = GetDlgItem(hwnd, IDC_EDIT1);
				char text[31]{};
				GetWindowTextA(textBox, text,30);
				
				if(!CheckTextEmpty(text))
				{
					strcpy_s<30>(User_Nickname, text);
					EndDialog(hwnd, IDOK);
				}
				else MessageBoxA(hwnd, "Username cannot be empty!", "Warning!", MB_ICONWARNING | MB_OK);
			}
			if(wParam == IDCANCEL) EndDialog(hwnd, IDCANCEL);
			return 1;
		}
		default:
			return 0;
	}
}

auto AppendChatBox(const std::string& ctext)
{
	std::string text(ctext);
	text += "\r\n";

	int index = GetWindowTextLength(chatbox);
	EnableWindow(chatbox, true);
	SetFocus(chatbox);
	SendMessageA(chatbox, EM_SETSEL, (WPARAM)index, (LPARAM)index);
	SendMessageA(chatbox, EM_REPLACESEL, 0, reinterpret_cast<LPARAM>(text.c_str()));
	EnableWindow(chatbox, false);
}

auto WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			FillRect(ps.hdc, &ps.rcPaint, reinterpret_cast<HBRUSH>(COLOR_WINDOW));
			EndPaint(hwnd, &ps);
			break;
		}
		case WM_COMMAND:
		{
			if(LOWORD(wParam) == BN_CLICKED)
			{
				if(lParam == reinterpret_cast<LPARAM>(sendButton))
				{
					char tmsg[501]{};
					GetWindowTextA(textbox, tmsg, 500);
					std::string cmsg(User_Nickname);
					cmsg += ":";
					cmsg += tmsg;
					try
					{
						if(client != nullptr) client->SendMessage(cmsg);
						else if(server != nullptr) server->SendMessage(cmsg);
						AppendChatBox(cmsg);
					}
					catch(std::exception& e)
					{
						MessageBoxA(AppWindow, e.what(), "Error sending message", MB_ICONERROR | MB_OK);
					}
				}
				return LRESULT(1);
			}
			switch(wParam)
			{
				case Menu_NewChat:
				{
					char* ip = reinterpret_cast<char*>(DialogBoxParamA(AppInstance, MAKEINTRESOURCEA(IDD_DIALOG3), hwnd, NewChat_WndProc, 0));
					if(ip != (char*)IDCANCEL)
					{
						EnableWindow(chatbox, TRUE);
						EnableWindow(textbox, TRUE);
						EnableWindow(sendButton, TRUE);

						try
						{
							client = new Client(User_Nickname, ip);
						}
						catch(std::exception& e)
						{
							MessageBoxA(AppWindow, e.what(), "Error!", MB_ICONERROR | MB_OK);
							return LRESULT(0);
						}
						serverOn = false;

						std::thread clientThrd(
						[]()
						{
							std::string msg(" ");
							
							while(true)
							{
								try
								{
									std::string msg = client->WaitForMessage();
									if(msg.size() > 0) AppendChatBox(msg);
								}
								catch(ConnectionClosedException&)
								{
									break;
								}
								catch(std::exception& e)
								{
									AppendChatBox(std::string("Error :") + e.what());
									break;
								}
							}

						});
						clientThrd.detach();
					}
					break;
				}
				case Menu_ExitOption:
					PostQuitMessage(0);
					break;
				case Menu_SetNickName:
					DialogBoxParamA(AppInstance, MAKEINTRESOURCEA(IDD_DIALOG2), hwnd, SetNickName_WndProc, 0);
					break;
				case Menu_NetworkingSettings:
					break;
				case Menu_About:
					DialogBoxParamA(AppInstance, MAKEINTRESOURCEA(IDD_DIALOG1), hwnd, AboutDialog_WndProc, 0);
					break;
				default:
					break;
			}
			break;
		}
		case WM_SIZE:
		{
			
			break;
		}
		case WM_QUIT:
		case WM_CLOSE:
			exit(0);
			break;
	}
	return DefWindowProcA(hwnd, msg, wParam, lParam);
}

auto App_cleanup()
{
	int errc = WSACleanup();
	if(errc != 0) throw std::exception("WSACleanup failed!");
}

auto WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR cmdArgs, int nShowCmd) -> int
{
	atexit(App_cleanup);
	AppInstance = hInstance;
	
	WNDCLASSA wndclass{};
	wndclass.hInstance = hInstance;
	wndclass.lpszClassName = AppClassName;
	wndclass.lpfnWndProc = WndProc;
	
	ATOM a = RegisterClassA(&wndclass);
	if(a == 0)
	{
		MessageBoxA(nullptr, "Failed to register the window class!", "Initialization error", MB_ICONERROR);
		return -1;
	}

	HMENU windowFile = CreateMenu();
	AppendMenuA(windowFile, MF_STRING, Menu_NewChat, "New conversation");
	AppendMenuA(windowFile, MF_STRING, Menu_ExitOption, "Exit");

	HMENU settingsMenu = CreateMenu();
	AppendMenuA(settingsMenu, MF_STRING, Menu_SetNickName, "Set nickname");
	AppendMenuA(settingsMenu, MF_STRING, Menu_NetworkingSettings, "Network settings");

	HMENU menu = CreateMenu();
	AppendMenuA(menu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(windowFile), "Window");
	AppendMenuA(menu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(settingsMenu), "Settings");
	AppendMenuA(menu, MF_STRING | MF_POPUP, Menu_About, "About");

	AppWindow = CreateWindowExA(
		0,
		AppClassName,
		"Small Chat Application",
		WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		400,
		300,
		nullptr,
		menu,
		hInstance,
		nullptr
	);

	if(AppWindow == nullptr)
	{
		MessageBoxA(nullptr, "Failed to create window!", "Error!", MB_ICONERROR);
		return -2;
	}

	textbox = CreateWindowExA(
		0,
		"EDIT",
		"",
		WS_VISIBLE | WS_CHILD | WS_DISABLED | WS_BORDER,
		2,
		220,
		330,
		20,
		AppWindow,
		nullptr,
		hInstance,
		nullptr
	);
	if(textbox == nullptr)
	{
		MessageBoxA(AppWindow, "Failed to create the textbox control!", "Error!", MB_ICONERROR | MB_OK);
		return -3;
	}

	chatbox = CreateWindowExA(
		0,
		"EDIT",
		"",
		WS_VISIBLE | WS_CHILD | WS_DISABLED | WS_BORDER | ES_MULTILINE | ES_LEFT | ES_READONLY,
		2,
		2,
		370,
		210,
		AppWindow,
		nullptr,
		hInstance,
		nullptr
	);
	if(chatbox == nullptr)
	{
		MessageBoxA(AppWindow, "Failed to create the chatbox control.", "Error", MB_ICONERROR | MB_OK);
		return -4;
	}

	sendButton = CreateWindowExA(
		0,
		"BUTTON",
		"Send",
		WS_VISIBLE | BS_PUSHBUTTON | WS_CHILD | WS_DISABLED,
		340,
		215,
		40,
		25,
		AppWindow,
		nullptr,
		hInstance,
		nullptr
	);
	if(sendButton == nullptr)
	{
		MessageBoxA(AppWindow, "Failed to create the send button control!", "Error", MB_ICONERROR | MB_OK);
		return -5;
	}

	WSADATA wsa_data{};
	int err1 = WSAStartup(MAKEWORD(2, 2),&wsa_data);
	if(err1 != 0)
	{
		MessageBoxA(AppWindow, std::format("WSAStartup failed with {} {}", err1, WSAGetLastError()).c_str(), "Initialization error", MB_ICONERROR | MB_OK);
		return -6;
	}

	if(AppWindow == nullptr)
	{
		MessageBoxA(nullptr, "Failed to create the window!", "Error!", MB_ICONERROR);
		return -7;
	}

	while(strlen(User_Nickname) == 0)
	{
		DialogBoxParamA(AppInstance, MAKEINTRESOURCEA(IDD_DIALOG2), AppWindow, SetNickName_WndProc, 0);
	}

	bool canCreateServer = true;
	try
	{
		server = new Server(User_Nickname);
	}
	catch(...)
	{
		canCreateServer = false;
		if(server != nullptr) delete server;
		server = nullptr;
	}

	if(canCreateServer && server != nullptr)
	{
		std::thread thrd_waitForMsgs([]() -> void
		{
		try
		{
			server->WaitForClient();
		}
		catch(std::exception& e)
		{
			MessageBoxA(AppWindow, e.what(), "Error", MB_ICONERROR | MB_OK);
			delete server;
			server = nullptr;
			throw;
		}
		EnableWindow(chatbox, TRUE);
		EnableWindow(textbox, TRUE);
		EnableWindow(sendButton, TRUE);
		while(serverOn)
		{
			try
			{
				std::string msg = server->WaitForMessage();
				if(msg.size() > 0)
				{
					AppendChatBox(msg);
				}
			}
			catch(ConnectionClosedException&)
			{
				delete server;
				server = nullptr;
				return;
			}
			catch(std::exception& e)
			{
				MessageBoxA(AppWindow, e.what(), "Error", MB_ICONERROR | MB_OK);
				delete server;
				server = nullptr;
				return;
			}
		}
		delete server;
		server = nullptr;
		});
		thrd_waitForMsgs.detach();
	}

	MSG msg{};
	while(GetMessageA(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	if(client != nullptr) delete client;
	client = nullptr;
	return 0;
}