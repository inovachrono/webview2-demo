#ifndef WIN32_WEBVIEW_DEMO
#define WIN32_WEBVIEW_DEMO

#include "nova_base.h"
#include <glfw/glfw3.h>
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c
#include <wil/com.h>
// include WebView2 header
#include <WebView2.h>

struct WebviewRect
{
    s32 left;
    s32 top;
    s32 width;
    s32 height;
};

struct WebviewData
{
    wchar_t *default_url;
    wil::com_ptr<ICoreWebView2Controller> controller;   // Pointer to webview_controller
    wil::com_ptr<ICoreWebView2> window;                 // Pointer to WebView window
};

void create_webview2_environment(WebviewData &webview_data, GLFWwindow *window, WebviewRect &webview_rect);

s32 navigate_to_url(WebviewData &webview_data, const wchar_t* url);

void set_webview2_position_size(WebviewData &webview_data, WebviewRect &webview_rect);

// #define WIN32_WEBVIEW_DEMO_IMPLEMENTATION
#endif  // WIN32_WEBVIEW_DEMO

#ifdef WIN32_WEBVIEW_DEMO_IMPLEMENTATION

#include <string>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#undef GLFW_EXPOSE_NATIVE_WIN32

#include <wrl.h>
#include <WebView2EnvironmentOptions.h>  // to use default classes inheriting from webview2 interfaces

void create_webview2_environment(WebviewData &webview_data, GLFWwindow *window, WebviewRect &webview_rect)
{
    // <-- WebView2 sample code starts here -->
    // Step 3 - Create a single WebView within the parent window
    // Locate the browser and set up the environment for WebView
    HWND hWnd = glfwGetWin32Window(window);
    auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
    // Flag to mute audio  --mute-audio
    // Flag to auto play audio/video  --autoplay-policy=no-user-gesture-required
    options->put_AdditionalBrowserArguments(L"--autoplay-policy=no-user-gesture-required");
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, options.Get(),
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hWnd, &webview_data, &webview_rect](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

                // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
                env->CreateCoreWebView2Controller(hWnd, Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [hWnd, &webview_data, &webview_rect](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                        if (controller != nullptr) {
                            webview_data.controller = controller;
                            webview_data.controller->get_CoreWebView2(&webview_data.window);
                        }

                        // Add a few settings for the webview
                        // The demo step is redundant since the values are the default settings
                        ICoreWebView2Settings* Settings;
                        webview_data.window->get_Settings(&Settings);
                        Settings->put_IsScriptEnabled(TRUE);
                        Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                        Settings->put_IsWebMessageEnabled(TRUE);

                        // Resize WebView to fit the bounds of the parent window
                        // GetClientRect(hWnd, &bounds);
                        // Resize the bounds such that the browser page window is not visible
                        set_webview2_position_size(webview_data, webview_rect);

                        // Schedule an async task to navigate to Bing
                        if(webview_data.default_url)
                        {
                            webview_data.window->Navigate(webview_data.default_url);
                        }
                        else
                        {
                            webview_data.window->Navigate(L"https://www.youtube.com/");
                        }

                        // Step 4 - Navigation events
                        // register an ICoreWebView2NavigationStartingEventHandler to cancel any non-https navigation
                        EventRegistrationToken token;
                        webview_data.window->add_NavigationStarting(Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
                            [](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                PWSTR uri;
                                args->get_Uri(&uri);
                                std::wstring source(uri);
                                if (source.substr(0, 5) != L"https") {
                                    args->put_Cancel(true);
                                }
                                CoTaskMemFree(uri);
                                return S_OK;
                            }).Get(), &token);

                        return S_OK;
                    }).Get());
                return S_OK;
            }).Get());



    // <-- WebView2 sample code ends here -->
}

// Go to a different different in webview2
s32 navigate_to_url(WebviewData &webview_data, const wchar_t *url)
{
    s32 success = -1;
    if(*url)
    {
        uint64_t startTime = GetTickCount64();
        uint64_t endTime = GetTickCount64();
        s32 timeLimit = 5000;  // wait for 10 seconds
        while ((endTime - startTime) < timeLimit)
        {
            if (webview_data.controller)
            {
                webview_data.window->Navigate(url);
                success = 1;
                break;
            }

            endTime = GetTickCount64();
        }
    }
    else
    {
        printf("\nEmpty url to play\n");
    }
    return success;
}

// change webview2 position and size
void set_webview2_position_size(WebviewData &webview_data, WebviewRect &webview_rect)
{
    RECT bounds;

    if (!webview_rect.width)
    {
        webview_rect.left = 0;
        webview_rect.width = 1;
    }
    if (!webview_rect.height)
    {
        webview_rect.top = 0;
        webview_rect.height = 1;
    }

    bounds.top = webview_rect.top;
    bounds.left = webview_rect.left;
    bounds.bottom = bounds.top + webview_rect.height;
    bounds.right = bounds.left + webview_rect.width;

    if(webview_data.controller)
        webview_data.controller->put_Bounds(bounds);
}

#endif  // WIN32_WEBVIEW_DEMO_IMPLEMENTATION
