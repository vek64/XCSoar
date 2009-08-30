/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "XCSoar.h"
#include "Version.hpp"
#include "Protection.hpp"
#include "Interface.hpp"
#include "LogFile.hpp"
#include "Compatibility/vk.h"
#include "Compatibility/string.h"
#include "MapWindow.h"
#include "Device/Parser.h"
#include "Calculations.h"
#include "Calculations2.h"
#include "Persist.hpp"
#include "Task.h"
#include "Dialogs.h"
#include "Language.hpp"
#include "Trigger.hpp"
#include "Message.h"
#include "Message.h"
#include "SettingsUser.hpp"
#include "Math/SunEphemeris.hpp"
#include "UtilsProfile.hpp"
#include "UtilsFLARM.hpp"
#include "UtilsSystem.hpp"
#include "LocalPath.hpp"
#include "Math/FastMath.h"
#include "Registry.hpp"
#include "Device/Port.h"
#include "Waypointparser.h"
#include "Airspace.h"
#include "Logger.h"
#include "McReady.h"
#include "AirfieldDetails.h"
#include "ButtonLabel.h"
#include "SnailTrail.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/MainWindow.hpp"
#include "Polar/Historical.hpp"
#include "ProcessTimer.hpp"

#include <commctrl.h>
#if (WINDOWSPC<1)
#include <sipapi.h>
#if !defined(CECORE) || UNDER_CE >= 300 || _WIN32_WCE >= 0x0300
#include <aygshell.h>
#endif
#endif

#include "TopologyStore.h"
#include "TerrainRenderer.h"
#include "Marks.h"
#include "Audio/VarioSound.h"
#include "Device/device.h"
#include "InputEvents.h"
#include "Atmosphere.h"
#include "Device/Geoid.h"

#include "InfoBoxLayout.h"
#include "InfoBox.h"
#include "InfoBoxManager.h"

#include "RasterTerrain.h"
#include "RasterWeather.h"

#include "Gauge/GaugeCDI.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeVarioAltA.hpp"

#include "Asset.hpp"

#include <assert.h>

///////////////////////////////////////////////////////////////////////////////
HINSTANCE hInst; // The current instance
MainWindow main_window;
MapWindow map_window;
GaugeVario *gauge_vario;
GaugeFLARM *gauge_flarm;
NMEA_INFO     GPS_INFO;
DERIVED_INFO  CALCULATED_INFO;

////////////////////////////////////////////////////////////////////////////////
//Local Static data
static int iTimerID= 0;

HBRUSH hBrushSelected;
HBRUSH hBrushUnselected;
HBRUSH hBrushButton;

#if (((UNDER_CE >= 300)||(_WIN32_WCE >= 0x0300)) && (WINDOWSPC<1))
#define HAVE_ACTIVATE_INFO
#endif

#ifdef HAVE_ACTIVATE_INFO
static SHACTIVATEINFO s_sai;
#endif

////////////////////////////////////////////////////////////////////////////////
// Forward declarations of functions included in this code module:
bool Startup (HINSTANCE, LPTSTR lpCmdLine);

////////////////////////////////////////////////////////////////////////////////


WPARAM WindowsMessageLoop() {
#if _DEBUG
 // _crtBreakAlloc = -1;     // Set this to the number in {} brackets to
                             // break on a memory leak
#endif

  HACCEL hAccelTable = LoadAccelerators(hInst, (LPCTSTR)IDC_XCSOAR);

  // Main message loop:
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
    {
      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
    }

#if (WINDOWSPC>0)
#if _DEBUG
  _CrtCheckMemory();
  _CrtDumpMemoryLeaks();
#endif
#endif

  return msg.wParam;
}



int WINAPI WinMain(     HINSTANCE hInstance,
                        HINSTANCE hPrevInstance,
                        LPTSTR    lpCmdLine,
                        int       nCmdShow)
{
  (void)hPrevInstance;

  InitAsset();

  Version();
  StartupStore(TEXT("Starting XCSoar %s\n"), XCSoar_Version);

  XCSoarGetOpts(lpCmdLine);

  InitCommonControls();

  StartupStore(TEXT("Initialise application instance\n"));

  // Perform application initialization:
  if (!Startup (hInstance, lpCmdLine))
    {
      return FALSE;
    }

  return WindowsMessageLoop();
}

/////////////////////////////////////////////////////////////////////////////////

static void PreloadInitialisation(bool ask) {
  if (ask) {
#ifdef PNA
    CleanRegistry(); // VENTA2-FIX for PNA we can't delete all registries..by now
#endif
  }

  SetToRegistry(TEXT("XCV"), 1);

#ifdef DEBUG_TRANSLATIONS
  ReadLanguageFile();
#endif

  // Registery (early)

  if (ask) {
    RestoreRegistry();
    ReadRegistrySettings();
    Message::InitFile();

    //    CreateProgressDialog(gettext(TEXT("Initialising")));

  } else {
    dlgStartupShowModal();
    RestoreRegistry();
    ReadRegistrySettings();

    CreateProgressDialog(gettext(TEXT("Initialising")));
  }

  // Interface (before interface)
  if (!ask) {
#ifndef DEBUG_TRANSLATIONS
    ReadLanguageFile();
#endif
    Message::LoadFile();
    InputEvents::readFile();
  }

}


static void AfterStartup() {
  static bool first = true;
  if (!first) {
    return;
  }
  first = false;
  StartupStore(TEXT("ProgramStarted=3\n"));
  StartupLogFreeRamAndStorage();

  Message::Startup(true);
#ifdef _SIM_
  StartupStore(TEXT("GCE_STARTUP_SIMULATOR\n"));
  InputEvents::processGlideComputer(GCE_STARTUP_SIMULATOR);
#else
  StartupStore(TEXT("GCE_STARTUP_REAL\n"));
  InputEvents::processGlideComputer(GCE_STARTUP_REAL);
#endif

  // Create default task if none exists
  StartupStore(TEXT("Create default task\n"));
  DefaultTask();

  StartupStore(TEXT("CloseProgressDialog\n"));
  CloseProgressDialog();

  main_window.full_screen();
  TriggerAll();
  InfoBoxManager::SetDirty(true);
  TriggerRedraws();

  Message::Startup(false);
#ifdef _INPUTDEBUG_
  InputEvents::showErrors();
#endif
}


bool Startup(HINSTANCE hInstance, LPTSTR lpCmdLine)
{
  TCHAR szTitle[MAX_LOADSTRING];                        // The title bar text
  TCHAR szWindowClass[MAX_LOADSTRING];                  // The window class name
  RECT rc;

  hInst = hInstance;            // Store instance handle in our global variable

  LoadString(hInstance, IDC_XCSOAR, szWindowClass, MAX_LOADSTRING);
  LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);

  //If it is already running, then focus on the window
  if (main_window.find(szWindowClass, szTitle))
    return false;

  main_window.register_class(hInst, szWindowClass);

  // other startup...

  InitSineTable();
  PreloadInitialisation(true);

  StartupStore(TEXT("Create main window\n"));

  RECT WindowSize = SystemWindowSize();
  main_window.set(szWindowClass, szTitle,
		  WindowSize.left, WindowSize.top,
		  WindowSize.right, WindowSize.bottom);

  if (!main_window.defined()) {
    return false;
  }

  rc = main_window.get_client_rect();
#if (WINDOWSPC>0)
  rc.left = 0;
  rc.right = SCREENWIDTH;
  rc.top = 0;
  rc.bottom = SCREENHEIGHT;
#endif

  StartupStore(TEXT("InfoBox geometry\n"));

  InfoBoxLayout::ScreenGeometry(rc);
  // color/pattern chart (must have infobox geometry before this)
  MapGfx.Initialise();

  hBrushSelected = (HBRUSH)CreateSolidBrush(MapGfx.ColorSelected);
  hBrushUnselected = (HBRUSH)CreateSolidBrush(MapGfx.ColorUnselected);
  hBrushButton = (HBRUSH)CreateSolidBrush(MapGfx.ColorButton);

  StartupStore(TEXT("Create info boxes\n"));
  RECT rcsmall = InfoBoxManager::Create(rc);

  StartupStore(TEXT("Create button labels\n"));
  ButtonLabel::CreateButtonLabels(rc);
  ButtonLabel::SetLabelText(0,TEXT("MODE"));

  StartupStore(TEXT("Initialise fonts\n"));
  InitialiseFonts(main_window, rc);

  StartupStore(TEXT("Create FLARM gauge\n"));
  gauge_flarm = new GaugeFLARM(main_window);

  StartupStore(TEXT("Initialise message system\n"));
  Message::Initialize(rc); // creates window, sets fonts

  ///////////////////////////////////////////////////////
  //// create map window

  StartupStore(TEXT("Create map window\n"));

  MapWindow::SetMapRect(rcsmall);
  map_window.register_class(hInst, TEXT("MapWindowClass"));
  map_window.set(main_window, TEXT("MapWindowClass"),
                 0, 0, rc.right - rc.left, rc.bottom-rc.top);

  map_window.set_font(MapWindowFont);

  ///////////////////////////////////////////////////////
  // initial show
  map_window.show();
  map_window.update();

  main_window.show();
  main_window.update();

  ///////////////////////////////////////////////////////
  // other initialisation...

#ifdef HAVE_ACTIVATE_INFO
  SHSetAppKeyWndAssoc(VK_APP1, main_window);
  SHSetAppKeyWndAssoc(VK_APP2, main_window);
  SHSetAppKeyWndAssoc(VK_APP3, main_window);
  SHSetAppKeyWndAssoc(VK_APP4, main_window);
  // Typical Record Button
  //	Why you can't always get this to work
  //	http://forums.devbuzz.com/m_1185/mpage_1/key_/tm.htm
  //	To do with the fact it is a global hotkey, but you can with code above
  //	Also APPA is record key on some systems
  SHSetAppKeyWndAssoc(VK_APP5, main_window);
  SHSetAppKeyWndAssoc(VK_APP6, main_window);
#endif

  // Initialise main blackboard data

  ClearTask();
  InitCalculations(&GPS_INFO,&CALCULATED_INFO);
  LinkGRecordDLL(); // try to link DLL if it exists
  OpenGeoid();

  PreloadInitialisation(false);
  ////////////////////////////////////////////////////////

  GaugeCDI::Create();

  gauge_vario = new GaugeVario(main_window);

  GPS_INFO.NAVWarning = true; // default, no gps at all!

  GPS_INFO.SwitchState.AirbrakeLocked = false;
  GPS_INFO.SwitchState.FlapPositive = false;
  GPS_INFO.SwitchState.FlapNeutral = false;
  GPS_INFO.SwitchState.FlapNegative = false;
  GPS_INFO.SwitchState.GearExtended = false;
  GPS_INFO.SwitchState.Acknowledge = false;
  GPS_INFO.SwitchState.Repeat = false;
  GPS_INFO.SwitchState.SpeedCommand = false;
  GPS_INFO.SwitchState.UserSwitchUp = false;
  GPS_INFO.SwitchState.UserSwitchMiddle = false;
  GPS_INFO.SwitchState.UserSwitchDown = false;
  GPS_INFO.SwitchState.VarioCircling = false;

  SYSTEMTIME pda_time;
  GetSystemTime(&pda_time);
  GPS_INFO.Time  = pda_time.wHour*3600+pda_time.wMinute*60+pda_time.wSecond;
  GPS_INFO.Year  = pda_time.wYear;
  GPS_INFO.Month = pda_time.wMonth;
  GPS_INFO.Day	 = pda_time.wDay;
  GPS_INFO.Hour  = pda_time.wHour;
  GPS_INFO.Minute = pda_time.wMinute;
  GPS_INFO.Second = pda_time.wSecond;

#ifdef _SIM_
  #if _SIM_STARTUPSPEED
  GPS_INFO.Speed = _SIM_STARTUPSPEED;
  #endif
  #if _SIM_STARTUPALTITUDE
  GPS_INFO.Altitude = _SIM_STARTUPALTITUDE;
  #endif
#endif

#ifndef NDEBUG
  DebugStore("# Start\r\n");
#endif

  LoadWindFromRegistry();
  CalculateNewPolarCoef();
  StartupStore(TEXT("GlidePolar::UpdatePolar\n"));
  GlidePolar::UpdatePolar(false);

// VENTA-ADDON
#ifdef VENTA_DEBUG_KEY
  CreateProgressDialog(gettext(TEXT("DEBUG KEY MODE ACTIVE")));
  Sleep(2000);
#endif
#ifdef VENTA_DEBUG_EVENT
  CreateProgressDialog(gettext(TEXT("DEBUG EVENT MODE ACTIVE")));
  Sleep(2000);
#endif
#ifdef VENTA_NOREGFONT
  CreateProgressDialog(gettext(TEXT("NO REGISTRY FONT LOAD")));
  Sleep(2000);
#endif

#ifdef CREDITS_FIVV
  CreateProgressDialog(gettext(TEXT("Special ITA version")));
  Sleep(1000);
#endif

#ifdef PNA // VENTA-ADDON
  TCHAR sTmp[MAX_PATH];
  _stprintf(sTmp, TEXT("PNA MODEL=%s (%d)"), GlobalModelName, GlobalModelType);
  CreateProgressDialog(sTmp); Sleep(3000);
#endif // non PNA
#ifdef _SIM_
  CreateProgressDialog(TEXT("SIMULATION")); Sleep(2000);
#endif

#ifdef PNA
  if ( SetBacklight() == true )
    CreateProgressDialog(TEXT("AUTOMATIC BACKLIGHT CONTROL"));
  else
    CreateProgressDialog(TEXT("NO BACKLIGHT CONTROL"));
  Sleep(3000);

  // this should work ok for all pdas as well
  if ( SetSoundVolume() == true )
    CreateProgressDialog(TEXT("AUTOMATIC SOUND LEVEL CONTROL"));
  else
    CreateProgressDialog(TEXT("NO SOUND LEVEL CONTROL"));
  Sleep(3000);
#endif

  RasterTerrain::OpenTerrain();

  ReadWayPoints();
  InitWayPointCalc();

  ReadAirfieldFile();
  SetHome(false);

  RasterTerrain::ServiceFullReload(GPS_INFO.Latitude,
                                   GPS_INFO.Longitude);

  CreateProgressDialog(gettext(TEXT("Scanning weather forecast")));
  StartupStore(TEXT("RASP load\n"));
  RASP.Scan(GPS_INFO.Latitude, GPS_INFO.Longitude);

  ReadAirspace();
  SortAirspace();

  OpenTopology();
  InitialiseMarks();

  OpenFLARMDetails();

#ifndef DISABLEAUDIOVARIO
  /*
  VarioSound_Init();
  VarioSound_EnableSound(EnableSoundVario);
  VarioSound_SetVdead(SoundDeadband);
  VarioSound_SetV(0);
  VarioSound_SetSoundVolume(SoundVolume);
  */
#endif

  CreateProgressDialog(gettext(TEXT("Starting devices")));
  devStartup(lpCmdLine);

  // re-set polar in case devices need the data
  StartupStore(TEXT("GlidePolar::UpdatePolar\n"));
  GlidePolar::UpdatePolar(true);

  CreateProgressDialog(gettext(TEXT("Initialising display")));

  // just about done....

  DoSunEphemeris(GPS_INFO.Longitude, GPS_INFO.Latitude);

  // Finally ready to go
  StartupStore(TEXT("CreateDrawingThread\n"));
  MapWindow::CreateDrawingThread();
  Sleep(100);
  StartupStore(TEXT("ShowInfoBoxes\n"));
  InfoBoxManager::Show();

  StartupStore(TEXT("CreateCalculationThread\n"));
  CreateCalculationThread();
  Sleep(500);

  StartupStore(TEXT("AirspaceWarnListInit\n"));
  AirspaceWarnListInit();
  StartupStore(TEXT("dlgAirspaceWarningInit\n"));
  dlgAirspaceWarningInit();

  // find unique ID of this PDA
  ReadAssetNumber();

  // Da-da, start everything now
  StartupStore(TEXT("ProgramStarted\n"));

  // map gets initial focus
  map_window.set_focus();

  globalRunningEvent.trigger();

  return true;
}


void Shutdown(void) {
  int i;

  if(iTimerID) {
    KillTimer(main_window,iTimerID);
    iTimerID = 0;
  }

  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));
  StartHourglassCursor();

  StartupStore(TEXT("Entering shutdown...\n"));
  StartupLogFreeRamAndStorage();

  // turn off all displays
  globalRunningEvent.reset();

  StartupStore(TEXT("dlgAirspaceWarningDeInit\n"));
  dlgAirspaceWarningDeInit();
  StartupStore(TEXT("AirspaceWarnListDeInit\n"));
  AirspaceWarnListDeInit();

  CreateProgressDialog(gettext(TEXT("Shutdown, saving logs...")));
  // stop logger
  guiStopLogger(true);

  CreateProgressDialog(gettext(TEXT("Shutdown, saving profile...")));
  // Save settings
  StoreRegistry();

  // Stop sound

  StartupStore(TEXT("SaveSoundSettings\n"));
  SaveSoundSettings();

#ifndef DISABLEAUDIOVARIO
  //  VarioSound_EnableSound(false);
  //  VarioSound_Close();
#endif

  // Stop drawing
  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));

  // Stop calculating too (wake up)
  TriggerAll();

  StartupStore(TEXT("CloseDrawingThread\n"));
  MapWindowBase::CloseDrawingThread();

  // Clear data

  CreateProgressDialog(gettext(TEXT("Shutdown, saving task...")));
  StartupStore(TEXT("Save default task\n"));
  mutexTaskData.Lock();
  ResumeAbortTask(-1); // turn off abort if it was on.
  mutexTaskData.Unlock();
  SaveDefaultTask();

  StartupStore(TEXT("Clear task data\n"));

  ClearTask();
  CloseAirspace();
  CloseWayPoints();
  mutexTaskData.Unlock();

  CreateProgressDialog(gettext(TEXT("Shutdown, please wait...")));
  StartupStore(TEXT("CloseTerrainTopology\n"));

  RASP.Close();
  RasterTerrain::CloseTerrain();

  CloseTopology();

  CloseMarks();

  CloseTerrainRenderer();

  devShutdown();

  SaveCalculationsPersist(&CALCULATED_INFO);
#if (EXPERIMENTAL > 0)
  //  CalibrationSave();
#endif

  #if defined(GNAV) && !defined(PCGNAV)
    StartupStore(TEXT("Altair shutdown\n"));
    Sleep(2500);
    StopHourglassCursor();
    InputEvents::eventDLLExecute(TEXT("altairplatform.dll SetShutdown 1"));
    while(1) {
      Sleep(100); // free time up for processor to perform shutdown
    }
  #endif

  CloseFLARMDetails();

  // Kill windows

  StartupStore(TEXT("Close Gauges\n"));

  GaugeCDI::Destroy();
  delete gauge_vario;
  gauge_vario = NULL;
  delete gauge_flarm;
  gauge_flarm = NULL;

  StartupStore(TEXT("Close Messages\n"));
  Message::Destroy();

  StartupStore(TEXT("Destroy Info Boxes\n"));
  InfoBoxManager::Destroy();

  StartupStore(TEXT("Destroy Button Labels\n"));
  ButtonLabel::Destroy();

  StartupStore(TEXT("Delete Objects\n"));

  // Kill graphics objects

  DeleteObject(hBrushSelected);
  DeleteObject(hBrushUnselected);
  DeleteObject(hBrushButton);

  DeleteFonts();

  DeleteAirspace();

  StartupStore(TEXT("Close Progress Dialog\n"));

  CloseProgressDialog();

  StartupStore(TEXT("Close Calculations\n"));
  CloseCalculations();

  CloseGeoid();

  StartupStore(TEXT("Close Windows - map\n"));
  map_window.reset();
  StartupStore(TEXT("Close Windows - main \n"));
  main_window.reset();
  StartupStore(TEXT("Close Graphics\n"));
  MapGfx.Destroy();

#ifdef DEBUG_TRANSLATIONS
  StartupStore(TEXT("Writing missing translations\n"));
  WriteMissingTranslations();
#endif

  StartupLogFreeRamAndStorage();
  StartupStore(TEXT("Finished shutdown\n"));
  StopHourglassCursor();

#ifndef NDEBUG
  TCHAR foop[80];
  TASK_POINT wp;
  TASK_POINT *wpr = &wp;
  _stprintf(foop,TEXT("Sizes %d %d %d\n"),
	    sizeof(TASK_POINT),
	    ((long)&wpr->AATTargetLocked)-((long)wpr),
	    ((long)&wpr->Target)-((long)wpr)
	    );
  StartupStore(foop);
#endif
}


///////////////////////////////////////////////////////////////////////////
// Windows event handlers

LRESULT MainHandler_Command(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  int wmId, wmEvent;
  HWND wmControl;

  wmId    = LOWORD(wParam);
  wmEvent = HIWORD(wParam);
  wmControl = (HWND)lParam;

  if(wmControl != NULL) {
    if (globalRunningEvent.test()) {

      main_window.full_screen();

      if (InfoBoxManager::Click(wmControl)) {
	return FALSE;
      }

      Message::CheckTouch(wmControl);

      if (ButtonLabel::CheckButtonPress(wmControl)) {
        return TRUE; // don't continue processing..
      }

    }
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}


LRESULT CALLBACK MainHandler_Colour(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  long wdata = Window::get_userdata((HWND)lParam);
  switch(wdata) {
  case 0:
    SetBkColor((HDC)wParam, MapGfx.ColorUnselected);
    SetTextColor((HDC)wParam, MapGfx.ColorBlack);
    return (LRESULT)hBrushUnselected;
  case 1:
    SetBkColor((HDC)wParam, MapGfx.ColorSelected);
    SetTextColor((HDC)wParam, MapGfx.ColorBlack);
    return (LRESULT)hBrushSelected;
  case 2:
    SetBkColor((HDC)wParam, MapGfx.ColorUnselected);
    SetTextColor((HDC)wParam, MapGfx.ColorWarning);
    return (LRESULT)hBrushUnselected;
  case 3:
    SetBkColor((HDC)wParam, MapGfx.ColorUnselected);
    SetTextColor((HDC)wParam, MapGfx.ColorOK);
    return (LRESULT)hBrushUnselected;
  case 4:
    // black on light green
    SetBkColor((HDC)wParam, MapGfx.ColorButton);
    SetTextColor((HDC)wParam, MapGfx.ColorBlack);
    return (LRESULT)hBrushButton;
  case 5:
    // grey on light green
    SetBkColor((HDC)wParam, MapGfx.ColorButton);
    SetTextColor((HDC)wParam, MapGfx.ColorMidGrey);
    return (LRESULT)hBrushButton;
  }
}

LRESULT CALLBACK MainHandler_Timer(void)
{
  if (globalRunningEvent.test()) {
    AfterStartup();
#ifdef _SIM_
    SIMProcessTimer();
#else
    ProcessTimer();
#endif
  }
}


LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {
    case WM_ERASEBKGND:
      return TRUE; // JMW trying to reduce screen flicker
      break;
    case WM_COMMAND:
      return MainHandler_Command(hWnd, message, wParam, lParam);
      break;
    case WM_CTLCOLORSTATIC:
      return MainHandler_Colour(hWnd, message, wParam, lParam);
      break;
    case WM_ACTIVATE:
      if(LOWORD(wParam) != WA_INACTIVE) {
	SetWindowPos(main_window,HWND_TOP, 0, 0, 0, 0,
		     SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
#ifdef HAVE_ACTIVATE_INFO
	SHFullScreen(main_window,SHFS_HIDETASKBAR|SHFS_HIDESIPBUTTON|SHFS_HIDESTARTICON);
#endif
      }
#ifdef HAVE_ACTIVATE_INFO
      SHHandleWMActivate(hWnd, wParam, lParam, &s_sai, FALSE);
#endif
      break;
    case WM_SETTINGCHANGE:
#ifdef HAVE_ACTIVATE_INFO
      SHHandleWMSettingChange(hWnd, wParam, lParam, &s_sai);
#endif
      break;
    case WM_SETFOCUS:
      // JMW not sure this ever does anything useful..
      if (globalRunningEvent.test()) {
	InfoBoxManager::Focus();
      }
      break;
#if defined(GNAV) || defined(PCGNAV)
    case WM_KEYDOWN: // JMW was keyup
#else
    case WM_KEYUP: // JMW was keyup
#endif
      InterfaceTimeoutReset();
      /* DON'T PROCESS KEYS HERE WITH NEWINFOBOX, IT CAUSES CRASHES! */
      break;
    case WM_TIMER:
      MainHandler_Timer();
      break;
    case WM_CREATE:
      main_window.created(hWnd);

#ifdef HAVE_ACTIVATE_INFO
      memset (&s_sai, 0, sizeof (s_sai));
      s_sai.cbSize = sizeof (s_sai);
#endif
      if (iTimerID == 0) {
        iTimerID = SetTimer(hWnd,1000,500,NULL); // 2 times per second
      }
      break;
    case WM_CLOSE:
      if (CheckShutdown()) {
	Shutdown();
      }
      break;
    case WM_DESTROY:
      if (hWnd==main_window) {
        PostQuitMessage(0);
      }
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
  return 0;
}
