// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"

#include <CppWinRTIncludes.h>
#include <UI.Xaml.Controls.h>
#include "App.h"
#include "ReactPackageProvider.h"
#include "ReactPropertyBag.h"
#include "winrt/SampleLibraryCS.h"
#include "winrt/SampleLibraryCpp.h"

namespace winrt::SampleAppCpp::implementation {

/// <summary>
/// Initializes the singleton application object.  This is the first line of
/// authored code executed, and as such is the logical equivalent of main() or
/// WinMain().
/// </summary>
App::App() noexcept {
#if BUNDLE
  JavaScriptBundleFile(L"index.windows");
  InstanceSettings().UseWebDebugger(false);
  InstanceSettings().UseFastRefresh(false);
#else
  JavaScriptMainModuleName(L"index");
  InstanceSettings().UseWebDebugger(true);
  InstanceSettings().UseFastRefresh(true);
#endif

#if _DEBUG
  InstanceSettings().EnableDeveloperMenu(true);
#else
  InstanceSettings().EnableDeveloperMenu(false);
#endif

  ReactPropertyBag::Set(InstanceSettings().Properties(), ReactPropertyId<int>{L"Prop1"}, 42);
  ReactPropertyBag::Set(InstanceSettings().Properties(), ReactPropertyId<hstring>{L"Prop2"}, L"Hello World!");

  PackageProviders().Append(make<ReactPackageProvider>()); // Includes all modules in this project
  PackageProviders().Append(winrt::SampleLibraryCpp::ReactPackageProvider());
  PackageProviders().Append(winrt::SampleLibraryCS::ReactPackageProvider());

  InitializeComponent();
}

void App::OnLaunched(winrt::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs e) {
  base::OnLaunched(e);
  auto window = xaml::Window::Current();
  window.Activate();
  window.Content().as<xaml::Controls::Frame>().Navigate(winrt::xaml_typename<MainPage>(), box_value(e.Arguments()));
}
} // namespace winrt::SampleAppCpp::implementation
