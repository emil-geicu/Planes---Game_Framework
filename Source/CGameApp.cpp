//-----------------------------------------------------------------------------
// File: CGameApp.cpp
//
// Desc: Game Application class, this is the central hub for all app processing
//
// Original design by Adam Hoult & Gary Simmons. Modified by Mihai Popescu.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CGameApp Specific Includes
//-----------------------------------------------------------------------------
#include "CGameApp.h"
#include <time.h>
#include <thread>
extern HINSTANCE g_hInst;

//-----------------------------------------------------------------------------
// CGameApp Member Functions
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Name : CGameApp () (Constructor)
// Desc : CGameApp Class Constructor
//-----------------------------------------------------------------------------
CGameApp::CGameApp()
{
	// Reset / Clear all required values
	m_hWnd			= NULL;
	m_hIcon			= NULL;
	m_hMenu			= NULL;
	m_pBBuffer		= NULL;
	m_pPlayer		= NULL;
	ally_pPlayer	= NULL;
	m_LastFrameRate = 0;
}

//-----------------------------------------------------------------------------
// Name : ~CGameApp () (Destructor)
// Desc : CGameApp Class Destructor
//-----------------------------------------------------------------------------
CGameApp::~CGameApp()
{
	// Shut the engine down
	ShutDown();
}

//-----------------------------------------------------------------------------
// Name : InitInstance ()
// Desc : Initialises the entire Engine here.
//-----------------------------------------------------------------------------
bool CGameApp::InitInstance( LPCTSTR lpCmdLine, int iCmdShow )
{
	// Create the primary display device
	if (!CreateDisplay()) { ShutDown(); return false; }

	// Build Objects
	if (!BuildObjects()) 
	{ 
		MessageBox( 0, _T("Failed to initialize properly. Reinstalling the application may solve this problem.\nIf the problem persists, please contact technical support."), _T("Fatal Error"), MB_OK | MB_ICONSTOP);
		ShutDown(); 
		return false; 
	}

	// Set up all required game states
	SetupGameState();

	// Success!
	return true;
}

//-----------------------------------------------------------------------------
// Name : CreateDisplay ()
// Desc : Create the display windows, devices etc, ready for rendering.
//-----------------------------------------------------------------------------
bool CGameApp::CreateDisplay()
{
	LPTSTR			WindowTitle		= _T("GameFramework");
	LPCSTR			WindowClass		= _T("GameFramework_Class");
	USHORT			Width			= 800;
	USHORT			Height			= 600;
	RECT			rc;
	WNDCLASSEX		wcex;


	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= CGameApp::StaticWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= g_hInst;
	wcex.hIcon			= LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= WindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON));

	if(RegisterClassEx(&wcex)==0)
		return false;

	// Retrieve the final client size of the window
	::GetClientRect( m_hWnd, &rc );
	m_nViewX		= rc.left;
	m_nViewY		= rc.top;
	m_nViewWidth	= rc.right - rc.left;
	m_nViewHeight	= rc.bottom - rc.top;

	m_hWnd = CreateWindow(WindowClass, WindowTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, Width, Height, NULL, NULL, g_hInst, this);

	if (!m_hWnd)
		return false;

	// Show the window
	ShowWindow(m_hWnd, SW_SHOW);

	// Success!!
	return true;
}

//-----------------------------------------------------------------------------
// Name : BeginGame ()
// Desc : Signals the beginning of the physical post-initialisation stage.
//		From here on, the game engine has control over processing.
//-----------------------------------------------------------------------------
int CGameApp::BeginGame()
{
	MSG		msg;

	// Start main loop
	while(true) 
	{
		// Did we recieve a message, or are we idling ?
		if ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) 
		{
			if (msg.message == WM_QUIT) break;
			TranslateMessage( &msg );
			DispatchMessage ( &msg );
		} 
		else 
		{
			// Advance Game Frame.
			FrameAdvance();

		} // End If messages waiting
	
	} // Until quit message is receieved

	return 0;
}

//-----------------------------------------------------------------------------
// Name : ShutDown ()
// Desc : Shuts down the game engine, and frees up all resources.
//-----------------------------------------------------------------------------
bool CGameApp::ShutDown()
{
	// Release any previously built objects
	ReleaseObjects ( );
	
	// Destroy menu, it may not be attached
	if ( m_hMenu ) DestroyMenu( m_hMenu );
	m_hMenu		 = NULL;

	// Destroy the render window
	SetMenu( m_hWnd, NULL );
	if ( m_hWnd ) DestroyWindow( m_hWnd );
	m_hWnd		  = NULL;
	
	// Shutdown Success
	return true;
}

//-----------------------------------------------------------------------------
// Name : StaticWndProc () (Static Callback)
// Desc : This is the main messge pump for ALL display devices, it captures
//		the appropriate messages, and routes them through to the application
//		class for which it was intended, therefore giving full class access.
// Note : It is VITALLY important that you should pass your 'this' pointer to
//		the lpParam parameter of the CreateWindow function if you wish to be
//		able to pass messages back to that app object.
//-----------------------------------------------------------------------------
LRESULT CALLBACK CGameApp::StaticWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	// If this is a create message, trap the 'this' pointer passed in and store it within the window.
	if ( Message == WM_CREATE ) SetWindowLong( hWnd, GWL_USERDATA, (LONG)((CREATESTRUCT FAR *)lParam)->lpCreateParams);

	// Obtain the correct destination for this message
	CGameApp *Destination = (CGameApp*)GetWindowLong( hWnd, GWL_USERDATA );
	
	// If the hWnd has a related class, pass it through
	if (Destination) return Destination->DisplayWndProc( hWnd, Message, wParam, lParam );
	
	// No destination found, defer to system...
	return DefWindowProc( hWnd, Message, wParam, lParam );
}

//-----------------------------------------------------------------------------
// Name : DisplayWndProc ()
// Desc : The display devices internal WndProc function. All messages being
//		passed to this function are relative to the window it owns.
//-----------------------------------------------------------------------------
LRESULT CGameApp::DisplayWndProc( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
	static UINT			fTimer;	

	// Determine message type
	switch (Message)
	{
		case WM_CREATE:
			break;
		
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		
		case WM_SIZE:
			if ( wParam == SIZE_MINIMIZED )
			{
				// App is inactive
				m_bActive = false;
			
			} // App has been minimized
			else
			{
				// App is active
				m_bActive = true;

				// Store new viewport sizes
				m_nViewWidth  = LOWORD( lParam );
				m_nViewHeight = HIWORD( lParam );
		
			
			} // End if !Minimized

			break;

		case WM_LBUTTONDOWN:
			// Capture the mouse
			SetCapture( m_hWnd );
			GetCursorPos( &m_OldCursorPos );
			break;

		case WM_LBUTTONUP:
			// Release the mouse
			ReleaseCapture( );
			break;

		case WM_KEYDOWN:
			switch(wParam)
			{
			case VK_ESCAPE:
				PostQuitMessage(0);
				break;
			case VK_RETURN:
				fTimer = SetTimer(m_hWnd, 1, 250, NULL);
				m_pPlayer->Explode();
				break;
			case 0x51:
				fTimer = SetTimer(m_hWnd, 1, 250, NULL);
				ally_pPlayer->Explode();
				break;
			case VK_SPACE:
				fTimer = SetTimer(m_hWnd, 1, 250, NULL);
				break;
			case 'L':
				loadGame();
				break;
			case 'K':
				SaveGame();
				break;
			case 'O':
				m_pPlayer->RotateLeft(m_pBBuffer);
				break;
			case 'P':
				m_pPlayer->RotateRight(m_pBBuffer);
				break;
			case'N':
				ally_pPlayer->RotateLeft(m_pBBuffer);
				break;
			case'M':
				ally_pPlayer->RotateRight(m_pBBuffer);
				break;
			}
			break;

		case WM_TIMER:
			switch(wParam)
			{
			case 1:
				if(!m_pPlayer->AdvanceExplosion())
					KillTimer(m_hWnd, 1);
				if(!ally_pPlayer->AdvanceExplosion())
					KillTimer(m_hWnd, 2);
			}
			break;

		case WM_COMMAND:
			break;

		default:
			return DefWindowProc(hWnd, Message, wParam, lParam);

	} // End Message Switch
	
	return 0;
}

//-----------------------------------------------------------------------------
// Name : BuildObjects ()
// Desc : Build our demonstration meshes, and the objects that instance them
//-----------------------------------------------------------------------------
bool CGameApp::BuildObjects()
{
	m_pBBuffer = new BackBuffer(m_hWnd, m_nViewWidth, m_nViewHeight);
	m_pPlayer = new CPlayer(m_pBBuffer,0);
	ally_pPlayer = new CPlayer(m_pBBuffer,400);

	if(!m_imgBackground.LoadBitmapFromFile("data/background.bmp", GetDC(m_hWnd)))
		return false;
	crates.push_front(new Crate(m_pBBuffer, rand() % 400));
	crates.push_front(new Crate(m_pBBuffer, rand() % 400));
	crates.push_front(new Crate(m_pBBuffer, rand() % 400));
	crates.push_front(new Crate(m_pBBuffer, rand() % 400));
	crates.push_front(new Crate(m_pBBuffer, rand() % 400));
	crates.push_front(new Crate(m_pBBuffer, rand() % 400));


	bonusLives.push_front(new BonusLives(m_pBBuffer, rand() % 400));
	

	enemies.push_front(new Enemy(m_pBBuffer, (int)m_nViewWidth, m_Timer.GetTimeElapsed()));
	enemies.push_front(new Enemy(m_pBBuffer, (int)m_nViewWidth / 2, m_Timer.GetTimeElapsed()));
	// Success!
	return true;
}

//-----------------------------------------------------------------------------
// Name : SetupGameState ()
// Desc : Sets up all the initial states required by the game.
//-----------------------------------------------------------------------------
void CGameApp::SetupGameState()
{
	srand(time(NULL));
	m_pPlayer->Position() = Vec2(100, 400);
	ally_pPlayer->Position() = Vec2(400, 300);
	for(auto& it: bullet)
		it->Position() = m_pPlayer->Position();
}

//-----------------------------------------------------------------------------
// Name : ReleaseObjects ()
// Desc : Releases our objects and their associated memory so that we can
//		rebuild them, if required, during our applications life-time.
//-----------------------------------------------------------------------------
void CGameApp::ReleaseObjects( )
{
	if(m_pPlayer != NULL)
	{
		delete m_pPlayer;
		m_pPlayer = NULL;
	}
	if (ally_pPlayer != NULL) {
		delete ally_pPlayer;
		ally_pPlayer= NULL;
	}

	if(m_pBBuffer != NULL)
	{
		delete m_pBBuffer;
		m_pBBuffer = NULL;
	}
}

//-----------------------------------------------------------------------------
// Name : FrameAdvance () (Private)
// Desc : Called to signal that we are now rendering the next frame.
//-----------------------------------------------------------------------------
void CGameApp::FrameAdvance()
{
	static TCHAR FrameRate[ 50 ];
	static TCHAR TitleBuffer[ 255 ];

	// Advance the timer
	m_Timer.Tick( );

	// Skip if app is inactive
	if ( !m_bActive ) return;
	
	// Get / Display the framerate
	if ( m_LastFrameRate != m_Timer.GetFrameRate() )
	{
		m_LastFrameRate = m_Timer.GetFrameRate( FrameRate, 50 );
		sprintf_s( TitleBuffer, _T("Game : %s"), FrameRate );
		SetWindowText( m_hWnd, TitleBuffer );

	} // End if Frame Rate Altered

	// Poll & Process input devices
	ProcessInput();

	// Animate the game objects
	AnimateObjects();

	// Drawing the game objects
	DrawObjects();
}

//-----------------------------------------------------------------------------
// Name : ProcessInput () (Private)
// Desc : Simply polls the input devices and performs basic input operations
//-----------------------------------------------------------------------------
void CGameApp::ProcessInput( )
{
	static UCHAR pKeyBuffer[ 256 ];
	ULONG		Direction = 0;
	ULONG		Direction2 = 0;
	POINT		CursorPos;
	float		X = 0.0f, Y = 0.0f;

	// Retrieve keyboard state
	if ( !GetKeyboardState( pKeyBuffer ) ) return;

	// Check the relevant keys
	if ( pKeyBuffer[ VK_UP	] & 0xF0 ) Direction |= CPlayer::DIR_FORWARD;
	if ( pKeyBuffer[ VK_DOWN  ] & 0xF0 ) Direction |= CPlayer::DIR_BACKWARD;
	if ( pKeyBuffer[ VK_LEFT  ] & 0xF0 ) Direction |= CPlayer::DIR_LEFT;
	if ( pKeyBuffer[ VK_RIGHT ] & 0xF0 ) Direction |= CPlayer::DIR_RIGHT;
	if (pKeyBuffer['W'] & 0xF0) Direction2 |= CPlayer::DIR_FORWARD;
	if (pKeyBuffer['S'] & 0xF0) Direction2 |= CPlayer::DIR_BACKWARD;
	if (pKeyBuffer['A'] & 0xF0) Direction2 |= CPlayer::DIR_LEFT;
	if (pKeyBuffer['D'] & 0xF0) Direction2 |= CPlayer::DIR_RIGHT;
	
	
	if (pKeyBuffer[VK_SPACE] & 0xF0) {
		
		Bullet* b = new Bullet(m_pBBuffer);
		bullet.push_back(b);
		m_pPlayer->FireBullet(b,m_pBBuffer);
	}
	
	if (pKeyBuffer['C'] & 0xF0) {

		Bullet* b = new Bullet(m_pBBuffer);
		bullet.push_back(b);
		ally_pPlayer->FireBullet(b, m_pBBuffer);
	}
	// Move the player

	m_pPlayer->Collsion(ally_pPlayer);
	ally_pPlayer->Collsion(m_pPlayer);
	m_pPlayer->Move(Direction);
	ally_pPlayer->Move(Direction2);

	
	
	
		

	// Now process the mouse (if the button is pressed)
	if ( GetCapture() == m_hWnd )
	{
		// Hide the mouse pointer
		SetCursor( NULL );

		// Retrieve the cursor position
		GetCursorPos( &CursorPos );

		// Reset our cursor position so we can keep going forever :)
		SetCursorPos( m_OldCursorPos.x, m_OldCursorPos.y );

	} // End if Captured
}

//-----------------------------------------------------------------------------
// Name : AnimateObjects () (Private)
// Desc : Animates the objects we currently have loaded.
//-----------------------------------------------------------------------------
void CGameApp::AnimateObjects()
{
	m_pPlayer->Update(m_Timer.GetTimeElapsed());
	ally_pPlayer->Update(m_Timer.GetTimeElapsed());

	for(auto &it:bullet)
		it->Update(m_Timer.GetTimeElapsed());

	for (auto& it : crates)
		it->Update(m_Timer.GetTimeElapsed(),600,800);

	for(auto &it:bonusLives)
		it->Update(m_Timer.GetTimeElapsed(), 600, 800);

	for (auto &it:enemies)
	{
		it->Update(m_Timer.GetTimeElapsed());
	}

	
	static int prev = 0;
	int current = clock();
	if (((current - prev) / CLOCKS_PER_SEC) > 2)
	{
		for (int i = 0; i < enemies.size(); i++)
		{
				enemies[i]->Shoot(m_pBBuffer);
			
		}
		prev = current;
	}

	ObjectCollision();
}

//-----------------------------------------------------------------------------
// Name : DrawObjects () (Private)
// Desc : Draws the game objects
//-----------------------------------------------------------------------------
void CGameApp::DrawObjects()
{
	m_pBBuffer->reset();


	static int lastTime ;
	int currentTime =clock();

	if (currentTime - lastTime > 20) {
		lastTime = currentTime;
		rollingBackgrondPos ++;
		if (rollingBackgrondPos <= m_nViewHeight)
			rollingBackgrondPos = -m_nViewHeight;
	}

	m_imgBackground.Paint(m_pBBuffer->getDC(), 0, rollingBackgrondPos);
	m_imgBackground.Paint(m_pBBuffer->getDC(), 0, m_nViewHeight+rollingBackgrondPos);



	

	//draw crates
	for (auto &it:crates)
		it->Draw();

	for (auto& it : bonusLives)
		it->Draw();

	if (!bullet.empty()) {
		for (auto it = bullet.begin(); it != bullet.end();)
		{
			if ((*it)->outsideScreen) {
				auto currentBullet = it;
				it++;
				delete *currentBullet;
				bullet.remove(*currentBullet);

			}
			else {
				(*it)->Draw();
				it++;
			}
		}

	}
	//drawing the enemies
	for (unsigned int i = 0; i < enemies.size(); i++)
		enemies[i]->Draw();

	m_pPlayer->Draw();
	ally_pPlayer->Draw();
	

	m_pBBuffer->present();
}
void CGameApp::loadGame() {
	f.open("SaveGame.txt", std::ifstream::in);
	int x1, x2, y1, y2;
	f >> x1 >> y1 >> x2 >> y2;
	m_pPlayer->Position() = Vec2(x1, y1);
	ally_pPlayer->Position() = Vec2(x2, y2);
	f.close();
}
void CGameApp::SaveGame() {
	
	g.open("SaveGame.txt", std::ofstream::out);
	g << (int)m_pPlayer->Position().x << " " << (int)m_pPlayer->Position().y << std::endl;
	g << (int)ally_pPlayer->Position().x << " " << (int)ally_pPlayer->Position().y << std::endl;
	g.close();
}


bool CGameApp::CollisionFlag(SpriteManipulation& obj1, SpriteManipulation& obj2)
{
	bool LowerLimit = obj1.Position().y + obj1.spriteHeight() / 2 < obj2.Position().y - obj2.spriteHeight() / 2;//Rect1.Bottom < Rect2.Top

	bool UpperLimit = obj1.Position().y - obj1.spriteHeight() / 2 > obj2.Position().y + obj2.spriteHeight() / 2;//Rect1.Top > Rect2.Bottom

	bool LeftLimit = obj1.Position().x - obj1.spriteWidth() / 2 > obj2.Position().x + obj2.spriteWidth() / 2;//Rect1.Left > Rect2.Right

	bool RightLlimit = obj1.Position().x + obj1.spriteWidth() / 2 < obj2.Position().x - obj2.spriteWidth() / 2;//Rect1.Right < Rect2.Left

	bool collided = !(LowerLimit|| UpperLimit ||  LeftLimit || RightLlimit);

	if (collided)
	{
		return true;
	}
	return false;
}

void CGameApp::ObjectCollision()
{
	static UINT	fTimer;
	srand(time(NULL));

	for (auto &i:crates)
	{
		if (CollisionFlag(*m_pPlayer, *i) && 
			!m_pPlayer->CurrentlyExploding()	   && 
			m_pPlayer->isAlive())
		{
			i->Position() = Vec2(rand() % 800, i->spriteHeight() / 2);
			fTimer = SetTimer(m_hWnd, 1, 100, NULL);
			m_pPlayer->Explode();
		}

		if (CollisionFlag(*ally_pPlayer, *i) && 
			!ally_pPlayer->CurrentlyExploding()		  && 
			ally_pPlayer->isAlive())
		{
			i->Position() = Vec2(rand() % 800, i->spriteHeight() / 2);
			fTimer = SetTimer(m_hWnd, 2, 100, NULL);
			ally_pPlayer->Explode();
		}
	}

	for (auto& i : bonusLives)
	{
		if (CollisionFlag(*m_pPlayer, *i) &&
			!m_pPlayer->CurrentlyExploding() &&
			m_pPlayer->isAlive())
		{
			i->Position() = Vec2(rand() % 800, i->spriteHeight() / 2);
			fTimer = SetTimer(m_hWnd, 1, 100, NULL);
			m_pPlayer->addLife(m_pBBuffer);
		}

		if (CollisionFlag(*ally_pPlayer, *i) &&
			!ally_pPlayer->CurrentlyExploding() &&
			ally_pPlayer->isAlive())
		{
			i->Position() = Vec2(rand() % 800, i->spriteHeight() / 2);
			fTimer = SetTimer(m_hWnd, 2, 100, NULL);
			ally_pPlayer->addLife(m_pBBuffer);
		}
	}

	
	for (auto &i:crates)
	{
		for (auto &it:bullet)
		{
			
			if (CollisionFlag(*i, *it)){

				i->Position() = Vec2(rand() % 800, i->spriteHeight() / 2);
				m_pPlayer->incrementScore(1);
				
			}
		}

	}

	for (auto& i : enemies)
	{
		if (CollisionFlag(*m_pPlayer, *i) &&
			!m_pPlayer->CurrentlyExploding() &&
			m_pPlayer->isAlive())
		{
			i->Position() = Vec2(rand() % 800, i->spriteHeight() / 2);
			fTimer = SetTimer(m_hWnd, 1, 100, NULL);
			m_pPlayer->Explode();
		}

		if (CollisionFlag(*ally_pPlayer, *i) &&
			!ally_pPlayer->CurrentlyExploding() &&
			ally_pPlayer->isAlive())
		{
			i->Position() = Vec2(rand() % 800, i->spriteHeight() / 2);
			fTimer = SetTimer(m_hWnd, 2, 100, NULL);
			ally_pPlayer->Explode();
		}
	}
	for (auto& i : enemies)
		for (auto& j : i->bullets)
		{
			if (CollisionFlag(*m_pPlayer, *j) &&
				!m_pPlayer->CurrentlyExploding() &&
				m_pPlayer->isAlive())
			{
				fTimer = SetTimer(m_hWnd, 1, 100, NULL);
				m_pPlayer->Explode();
			}

			if (CollisionFlag(*ally_pPlayer, *j) &&
				!ally_pPlayer->CurrentlyExploding() &&
				ally_pPlayer->isAlive())
			{
				fTimer = SetTimer(m_hWnd, 2, 100, NULL);
				ally_pPlayer->Explode();
			}
		}
	for (auto& i : enemies)
	{
		for (auto& it : bullet)
		{

			if (CollisionFlag(*i, *it)) {

				i->Position().x = rand() % 800;
				i->Position().y = 100;
				m_pPlayer->incrementScore(10);

			}
		}

	}
	
}