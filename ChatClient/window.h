#pragma once
//EXTERNAL INCLUDES
#include <Windows.h>
#include <vector>
#include <string>
//INTERNAL INCLUDES

typedef int64_t i64;

typedef uint32_t ui32;
typedef uint64_t ui64;

//Display struct
struct Display
{
	ui32 id;

	i64 posX;
	i64 posY;
	ui64 width;
	ui64 height;
};

class Window
{

public:
	enum class WindowState
	{
		Started,
		Closed
	};

	//Create window
	void Instantiate(ui32 width, ui32 height, ui32 displayID, const char* title);

	//Add a display
	void AddDisplay(Display& display);

	//Set window state
	void SetState(WindowState state);

	//Set messages
	void SetCurrentMessage(std::string message);
	void SetRecievedMessage(std::string message);

	//Spawn message
	void RecievedMessage(std::string message);

	//Get Display by ID
	Display* GetDisplay(ui32 displayID);

	//Get current window state
	WindowState GetState(void);

	//Get handle to main window
	HWND GetHandle();

	//Get states for messages
	bool& GetSendState();
	bool& GetRecievedState();

	//Get strings for messages
	std::string  GetCurrentMessage();
	std::string  GetRecievedMessage();

private:
	HWND handle = 0;
	WindowState state = WindowState::Started;

	ui32 width = 0;
	ui32 height = 0;

	std::string currentMessage;
	std::string recievedMessage;

	bool send = false;
	bool recieve = false;
};