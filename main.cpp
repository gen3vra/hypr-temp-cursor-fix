// Symptoms of the same bug
// - Cursor shape doesn't change when we close a window and only have the desktop under it
// - Set up two scrollable windows (Window A & B) with input:follow_mouse = 2 (click to focus)
//   Focus Window B and scroll Window A by placing mouse over it
//   Switch workspace away
//   Switch back
//   Scroll → no-op
//   Move mouse 1px → scroll works
// - Cursor shape doesn't change/reset on workspace switch if onto empty desktop space
//   The issue is that the compositor is not re-evaluating the surface mouse is under after these situations
// We fix all of these by simulating mouse movement or setting cursor manually after these events, and top it off with a periodic hacky "fix"
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/helpers/Monitor.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <chrono>

HANDLE PHANDLE = nullptr;

static CFunctionHook *g_pChangeWorkspaceHook1 = nullptr;
static CFunctionHook *g_pRenderMonitorHook = nullptr;
static CFunctionHook *g_pCloseWindowHook = nullptr;

static int g_iFramesUntilSimulate = -1;
static const int FRAMES_TO_WAIT = 5;

typedef void (*origChangeWorkspace1)(void *, const PHLWORKSPACE &, bool, bool,
                                     bool);
typedef void (*origRenderMonitor)(void *, PHLMONITOR, bool);

typedef void (*origCloseWindow)(void *, PHLWINDOW);

void hkCloseWindow(void *thisptr, PHLWINDOW window)
{
  (*(origCloseWindow)g_pCloseWindowHook->m_original)(thisptr, window);
  // Hack, would be better if it knew it was on desktop and set this
  g_pHyprRenderer->setCursorFromName("left_ptr");
}

void hkRenderMonitor(void *thisptr, PHLMONITOR pMonitor, bool commit)
{
  (*(origRenderMonitor)g_pRenderMonitorHook->m_original)(thisptr, pMonitor,
                                                         commit);
  // "Fix" it once and for all
  // Lightweight periodic action: ~500ms-ish
  static unsigned char frameTick = 0;
  if (++frameTick >= 30)
  { // ~30 frames @60Hz ≈ 500ms
    frameTick = 0;
    static const std::string LEFT_PTR = "left_ptr";
    // Same frame so never visually changes if needs to be something other than left_ptr, otherwise resets it any situation it'd need it
    g_pHyprRenderer->setCursorFromName(LEFT_PTR);
    g_pInputManager->simulateMouseMovement();
  }
  if (g_iFramesUntilSimulate < 0)
    return;
  if (g_iFramesUntilSimulate-- == 0)
  {
    g_pInputManager->simulateMouseMovement();
    g_iFramesUntilSimulate = -1;
  }
}

void hkChangeWorkspace1(void *thisptr, const PHLWORKSPACE &pWorkspace,
                        bool internal, bool noMouseMove, bool noFocus)
{
  (*(origChangeWorkspace1)g_pChangeWorkspaceHook1->m_original)(
      thisptr, pWorkspace, internal, noMouseMove, noFocus);
  g_iFramesUntilSimulate = FRAMES_TO_WAIT;
  // Hack to fix cursor on window shape not resetting when switching to workspace with empty desktop space under cursor
  g_pHyprRenderer->setCursorFromName("left_ptr");
}

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle)
{
  PHANDLE = handle;

  g_pChangeWorkspaceHook1 = HyprlandAPI::createFunctionHook(
      PHANDLE,
      (void *)static_cast<void (CMonitor::*)(const PHLWORKSPACE &, bool, bool,
                                             bool)>(&CMonitor::changeWorkspace),
      (void *)&hkChangeWorkspace1);
  g_pChangeWorkspaceHook1->hook();

  g_pRenderMonitorHook = HyprlandAPI::createFunctionHook(
      PHANDLE,
      (void *)static_cast<void (CHyprRenderer::*)(PHLMONITOR, bool)>(
          &CHyprRenderer::renderMonitor),
      (void *)&hkRenderMonitor);
  g_pRenderMonitorHook->hook();

  g_pCloseWindowHook = HyprlandAPI::createFunctionHook(
      PHANDLE, (void *)&CCompositor::closeWindow, (void *)&hkCloseWindow);
  g_pCloseWindowHook->hook();
  HyprlandAPI::addNotification(PHANDLE, "[cursor-fix] loaded!",
                               CHyprColor{0.2, 1.0, 0.2, 1.0}, 3000);
  return {"cursor-fix", "<3 hypr", "gen", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT()
{
  if (g_pCloseWindowHook)
    HyprlandAPI::removeFunctionHook(PHANDLE, g_pCloseWindowHook);
  if (g_pChangeWorkspaceHook1)
    HyprlandAPI::removeFunctionHook(PHANDLE, g_pChangeWorkspaceHook1);
  if (g_pRenderMonitorHook)
    HyprlandAPI::removeFunctionHook(PHANDLE, g_pRenderMonitorHook);
}
