//EXTERNAL INCLUDES
#include <unordered_map>
//INTERNAL INCLUDES
#include "window.h"

#define SendButton_ID 0 //ID for send button events.
#define TextBox_ID 2 //ID for send button events.

//Global variables needed outside of the window class.
std::vector<Display> g_displays;
std::unordered_map<HWND, Window*> g_windowMapping;

//Messages as the text on the window.
std::vector<HWND> messages;


HWND hwndText; //Textbox window.
HWND hwndChat; //Window where the chat is spawned in.

//Handles messages for a window.
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	//Handle the messages in the window.
	switch (msg)
	{
	case WM_LBUTTONDOWN: //If Left mouse button is pressed
		SetFocus(hwnd); //Set focus to the window.
		break;
	case WM_KEYDOWN: //If a key is pressed:
	{
		switch (wParam)
		{
		case 38: //Up button.
		{
			//Get the position of the lowest message.
			RECT Rect;
			GetWindowRect(messages[messages.size() - 1], &Rect);
			MapWindowPoints(HWND_DESKTOP, hwndChat, (LPPOINT)&Rect, 2);

			//If its not over the screen.
			if (true)
			{
				//Move all messages up by 3 units.
				for (HWND text : messages)
				{
					GetWindowRect(text, &Rect);
					MapWindowPoints(HWND_DESKTOP, hwndChat, (LPPOINT)&Rect, 2);
					SetWindowPos(text, 0, Rect.left, Rect.top - 3, Rect.right, Rect.bottom, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
				}
			}
			break;
		}
		case 40: //Down button.
		{
			//Get the position of the uppest message.
			RECT Rect;
			GetWindowRect(messages[0], &Rect);
			MapWindowPoints(HWND_DESKTOP, hwndChat, (LPPOINT)&Rect, 2);

			printf("Rect top: %d", Rect.top);

			//If its over the screen.
			if (Rect.top < 1)
			{
				//Move all messages down by 3 units.
				for (HWND text : messages)
				{
					GetWindowRect(text, &Rect);
					MapWindowPoints(HWND_DESKTOP, hwndChat, (LPPOINT)&Rect, 2);
					SetWindowPos(text, 0, Rect.left, Rect.top + 3, Rect.right, Rect.bottom, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
				}
			}
			break;
		}
		}
		break;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) //Convert command
		{
			case SendButton_ID: //If its the send button being pressed.
			{
				//Trigger event to send a message.
				g_windowMapping[hwnd]->GetSendState() = true;

				//Copy message from the textBox
				char* message = new char;
				GetWindowText(hwndText, message, 300);

				//Convert it into a string, clear the textBox and set the message as the message thats being send.
				std::string str = message;
				SetWindowText(hwndText, "");
				g_windowMapping[hwnd]->SetCurrentMessage(str);
				break;
			}
		}
		break;	
	}
	//If the message is destroy, destroy the window.
	case WM_DESTROY:
	{
		g_windowMapping[hwnd]->SetState(Window::WindowState::Closed); //Set closed state
		DestroyWindow(hwnd); //Destroy the window.
		break;
	}
		//If the message is close, stop proccessing messages.
	case WM_CLOSE:
	{
		g_windowMapping[hwnd]->SetState(Window::WindowState::Closed); //Set closed state
		PostQuitMessage(0);
		break;
	}
	default:
		//Go on.
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

//Save display information.
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	//Initialize variables.
	MONITORINFO info;
	info.cbSize = sizeof(info);

	//Get the monitor info of the inputed monitor.
	if (GetMonitorInfo(hMonitor, &info))
	{
		//Save the Monitor information into the struct display.
		Display currentMonitor;

		currentMonitor.width = abs(info.rcMonitor.left - info.rcMonitor.right);
		currentMonitor.height = abs(info.rcMonitor.top - info.rcMonitor.bottom);

		currentMonitor.posX = info.rcMonitor.left;
		currentMonitor.posY = info.rcMonitor.top;

		//Add the display into windows displays list.
		g_displays.push_back(currentMonitor);
	}

	return TRUE;
}


void Window::Instantiate(ui32 width, ui32 height, ui32 displayID, const char* title)
{
	//Initialize variable
	ui32 style = 0;
	this->width = width;
	this->height = height;

	//Enumerate through all monitors connected to the computer and return if there are none.
	if (!EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0))
		return;

	//Create a struct to hold window data and fill that struct with data.
	WNDCLASSEXA wndex = { 0 };
	wndex.cbSize = sizeof(WNDCLASSEX);
	wndex.lpfnWndProc = WndProc; //Set this function as the function to handle the messages.
	wndex.hInstance = GetModuleHandle(NULL);
	wndex.hIcon = NULL;
	wndex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndex.hbrBackground = CreateSolidBrush(RGB(255, 210, 0)); //Yellow
	wndex.lpszClassName = title;

	//Register a window and if that fails crash the program.
	if (!RegisterClassExA(&wndex))
		throw;

	//Set the window style.
	style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	//Get the selected display.
	Display* selectedDisplay = GetDisplay(displayID);

	//Get the center position of the display.
	i64 posX = ((selectedDisplay->width - width) / 2) + selectedDisplay->posX;
	i64 posY = ((selectedDisplay->height - height) / 2) + selectedDisplay->posY;

	//Fill the RECT struct with data.
	RECT adjustedRect = RECT
	{
		static_cast<LONG>(0),
		static_cast<LONG>(0),
		static_cast<LONG>(width),
		static_cast<LONG>(height)
	};

	//Adjust the Window. If that fails crash the program.
	if (!AdjustWindowRect(&adjustedRect, style, FALSE))
		throw;

	//Create a handle onto a newly created window.
	this->handle = CreateWindowExA
	(
		NULL,
		title,
		title,
		style,
		static_cast<int>(posX),
		static_cast<int>(posY),
		adjustedRect.right,
		adjustedRect.bottom,
		NULL,
		NULL,
		GetModuleHandle(NULL), NULL
	);

	//Create the SendButton
	HWND hwndButton = CreateWindowEx(0, "BUTTON", "Send Message",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 315, 480, 120, 25, this->handle,
		(HMENU)SendButton_ID, (HINSTANCE)GetWindowLong(this->handle, -6), NULL);

	//Create the textBox.
	hwndText = CreateWindowEx(0, "EDIT", "",
		WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_WANTRETURN | ES_MULTILINE,
		10, 480, 300, 25, this->handle, (HMENU)TextBox_ID, (HINSTANCE)GetWindowLong(this->handle, -6), 0);

	//Create the chat window.
	hwndChat = CreateWindowEx(0, "static", "",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP, 50, 50, 340, 400, this->handle,
		NULL, (HINSTANCE)GetWindowLong(this->handle, -6), NULL);

	//If that fails crash the program.
	if (this->handle == NULL)
		throw;

	//Set the window to the main window handle.
	g_windowMapping[this->handle] = this;

	//Set window state
	this->state = Window::WindowState::Started;
	
	//Show window and draw it.
	ShowWindow(this->handle, SW_SHOW);
	UpdateWindow(this->handle);
}

void Window::AddDisplay(Display& display)
{
	g_displays.push_back(display);
}

void Window::SetState(WindowState state)
{
	this->state = state;
}

void Window::SetCurrentMessage(std::string message)
{
	this->currentMessage = message;
}

void Window::SetRecievedMessage(std::string message)
{
	this->recievedMessage = message;
}

void Window::RecievedMessage(std::string message)
{
	HWND hwndChatText;
	RECT Rect;

	if (messages.size() == 0)
	{
		//Create message text at top position if its the first one.
		hwndChatText = CreateWindowEx(0, "static", message.c_str(),
			WS_CHILD | WS_VISIBLE | WS_TABSTOP, 1, 1, 500, 22, hwndChat,
			NULL, (HINSTANCE)GetWindowLong(hwndChat, -6), NULL);
	}
	else
	{
		//Get position of the lowest message.
		GetWindowRect(messages[messages.size() - 1], &Rect);
		MapWindowPoints(HWND_DESKTOP, hwndChat, (LPPOINT)&Rect, 2);

		//Create message text 22 units under the lowest message.
		hwndChatText = CreateWindowEx(0, "static", message.c_str(),
			WS_CHILD | WS_VISIBLE | WS_TABSTOP, 1, Rect.top + 22, 500, 22, hwndChat,
			NULL, (HINSTANCE)GetWindowLong(hwndChat, -6), NULL);

	}

	//Save message to global message vector.s
	messages.push_back(hwndChatText);

	//If spawned message is under the screen.
	if (Rect.top + 22 > 380)
	{
		//Move all messages up by top - 380 units.
		for (HWND text : messages)
		{
			RECT textRect;
			GetWindowRect(text, &textRect);
			MapWindowPoints(HWND_DESKTOP, hwndChat, (LPPOINT)&textRect, 2);
			SetWindowPos(text, 0, textRect.left, textRect.top - 22, textRect.right, textRect.bottom, SWP_NOZORDER | SWP_NOSIZE | SWP_SHOWWINDOW);
		}
	}
}

Window::WindowState Window::GetState()
{
	return this->state;
}

HWND Window::GetHandle()
{
	return this->handle;
}

bool & Window::GetSendState()
{
	return this->send;
}

bool & Window::GetRecievedState()
{
	return this->recieve;
}

std::string Window::GetCurrentMessage()
{
	return this->currentMessage;
}

std::string Window::GetRecievedMessage()
{
	return this->recievedMessage;
}

Display* Window::GetDisplay(ui32 displayID)
{
	return &g_displays[displayID];
}