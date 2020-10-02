// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include <cdebug.h>
#include <Views/ShadowNodeBase.h>
#include "ModalViewManager.h"
#include "TouchEventHandler.h"
#include <ReactRootView.h>
#include <Modules/NativeUIManager.h>
#include <UI.Xaml.Controls.Primitives.h>
#include <UI.Xaml.Controls.h>
#include <UI.Xaml.Documents.h>
#include <UI.Xaml.Input.h>
#include <UI.Xaml.Media.h>
#include <Utils/Helpers.h>
#include <Utils/ValueUtils.h>
#include <winrt/Windows.UI.Core.h>

namespace winrt {
using namespace xaml::Controls::Primitives;
} // namespace winrt

namespace react::uwp {

class ModalShadowNode : public ShadowNodeBase {
  using Super = ShadowNodeBase;

 public:
  ModalShadowNode() = default;
  virtual ~ModalShadowNode();

  void createView() override;
  void AddView(ShadowNode &child, int64_t index) override;
  virtual void removeAllChildren() override;
  virtual void RemoveChildAt(int64_t indexToRemove) override;
  void updateProperties(const folly::dynamic &&props) override;

  static void OnModalClosed(IReactInstance &instance, int64_t tag);
  winrt::Windows::Foundation::Size GetAppWindowSize();
  xaml::Controls::ContentControl GetControl();
  void UpdateTabStops();
  void UpdateLayout();

  bool IsWindowed() override {
    return true;
  }

 private:
  bool m_autoFocus{true};
  std::unique_ptr<TouchEventHandler> m_touchEventHandler;
  std::unique_ptr<PreviewKeyboardEventHandlerOnRoot> m_previewKeyboardEventHandlerOnRoot;

  int64_t m_targetTag{0};
  winrt::Popup::Closed_revoker m_popupClosedRevoker{};
  winrt::Popup::Opened_revoker m_popupOpenedRevoker{};
  xaml::FrameworkElement::SizeChanged_revoker m_popupSizeChangedRevoker{};
  xaml::Window::SizeChanged_revoker m_windowSizeChangedRevoker{};
};

ModalShadowNode::~ModalShadowNode() {
  m_touchEventHandler->RemoveTouchHandlers();
  m_previewKeyboardEventHandlerOnRoot->unhook();
}

xaml::Controls::ContentControl ModalShadowNode::GetControl() {
  auto popup = GetView().as<winrt::Popup>();
  auto control = popup.Child().as<xaml::Controls::Border>().Child().as<xaml::Controls::ContentControl>();

  return control;
}

void ModalShadowNode::createView() {
  Super::createView();

  auto popup = GetView().as<winrt::Popup>();
  auto control = GetControl();
  auto wkinstance = GetViewManager()->GetReactInstance();
  m_touchEventHandler = std::make_unique<TouchEventHandler>(wkinstance);
  m_previewKeyboardEventHandlerOnRoot = std::make_unique<PreviewKeyboardEventHandlerOnRoot>(wkinstance);

  m_popupClosedRevoker = popup.Closed(winrt::auto_revoke, [=](auto &&, auto &&) {
    auto instance = wkinstance.lock();
    if (!m_updating && instance != nullptr)
      OnModalClosed(*instance, m_tag);
  });

  m_popupOpenedRevoker = popup.Opened(winrt::auto_revoke, [=](auto &&, auto &&) {
    auto instance = wkinstance.lock();
    if (!m_updating && instance != nullptr) {
      // When multiple flyouts/popups are overlapping, XAML's theme shadows
      // don't render properly. As a workaround we enable a z-index
      // translation based on an elevation derived from the count of open
      // popups/flyouts. We apply this translation on open of the popup.
      // (Translation is only supported on RS5+, eg. IUIElement9)
      if (auto uiElement9 = GetView().try_as<xaml::IUIElement9>()) {
        auto numOpenPopups = CountOpenPopups();
        if (numOpenPopups > 0) {
          winrt::Numerics::float3 translation{0, 0, (float)16 * numOpenPopups};
          popup.Translation(translation);
        }
      }

      popup.XamlRoot().Changed([popup](xaml::XamlRoot xamlRoot, auto &&) {
        auto rootSize = xamlRoot.Size();
        popup.Child().as<xaml::FrameworkElement>().Width(rootSize.Width);
        popup.Child().as<xaml::FrameworkElement>().Height(rootSize.Height);
      });
      auto root = popup.Parent().as<xaml::FrameworkElement>();
      if (auto xr = root.XamlRoot()) {
        cwdebug << winrt::get_class_name(xr).c_str() << endl;
      }
      auto c = root;
      while (c = c.Parent().try_as<xaml::FrameworkElement>()) {
        auto classname = winrt::get_class_name(c);
        cwdebug << classname.c_str() << endl;
      }
      
      UpdateLayout();
      UpdateTabStops();
    }
  });

  m_popupSizeChangedRevoker = popup.SizeChanged(winrt::auto_revoke, [=](auto &&, auto &&) {
    auto instance = wkinstance.lock();
    if (!m_updating && instance != nullptr) {
      UpdateLayout();
    }
  });

  m_windowSizeChangedRevoker = xaml::Window::Current().SizeChanged(winrt::auto_revoke, [=](auto &&, auto &&) {
    auto instance = wkinstance.lock();
    if (!m_updating && instance != nullptr) {
      UpdateLayout();
    }
  });

  popup.IsOpen(true);
}

void ModalShadowNode::AddView(ShadowNode &child, int64_t index) {
  if (index != 0) {
    assert(false);
    return;
  }

  auto control = GetControl();
  auto childView = static_cast<ShadowNodeBase &>(child).GetView();

  control.Content(childView);

  m_touchEventHandler->AddTouchHandlers(childView);
  m_previewKeyboardEventHandlerOnRoot->hook(childView);
}

void ModalShadowNode::RemoveChildAt(int64_t indexToRemove) {
  if (indexToRemove != 0) {
    assert(false);
    return;
  }

  removeAllChildren();
}

void ModalShadowNode::removeAllChildren() {
  auto control = GetControl();

  control.Content(nullptr);

  m_touchEventHandler->RemoveTouchHandlers();
  m_previewKeyboardEventHandlerOnRoot->unhook();
}

void ModalShadowNode::updateProperties(const folly::dynamic &&props) {
  m_updating = true;

  auto popup = GetView().as<winrt::Popup>();
  if (popup == nullptr)
    return;

  bool needsLayoutUpdate = false;
  bool needsTabUpdate = false;
  const folly::dynamic *isOpenProp = nullptr;

  for (auto &pair : props.items()) {
    const std::string &propertyName = pair.first.getString();
    const folly::dynamic &propertyValue = pair.second;

    if (propertyName == "visible") {
      isOpenProp = &propertyValue;
    }
    else if (propertyName == "autoFocus") {
      if (propertyValue.isBool()) {
        m_autoFocus = propertyValue.asBool();
      } else if (propertyValue.isNull()) {
        m_autoFocus = true;
      }
      needsTabUpdate = true;
    } else if (propertyName == "target") {
      if (propertyValue.isNumber())
        m_targetTag = static_cast<int64_t>(propertyValue.asDouble());
      else
        m_targetTag = -1;
    } else if (propertyName == "isOpen") {
      isOpenProp = &propertyValue;
    } else if (propertyName == "isLightDismissEnabled") {
      if (propertyValue.isBool()) {
        popup.IsLightDismissEnabled(propertyValue.getBool());
      } else if (propertyValue.isNull()) {
        popup.ClearValue(winrt::Popup::IsLightDismissEnabledProperty());
      }
      needsTabUpdate = true;
    }
  }

  if (needsLayoutUpdate) {
    UpdateLayout();
  }
  if (needsTabUpdate) {
    UpdateTabStops();
  }

  // IsOpen needs to be set after IsLightDismissEnabled for light-dismiss to work
  if (isOpenProp != nullptr) {
    if (isOpenProp->isBool())
      popup.IsOpen(isOpenProp->getBool());
    else if (isOpenProp->isNull())
      popup.ClearValue(winrt::Popup::IsOpenProperty());
  }

  Super::updateProperties(std::move(props));
  m_updating = false;
}

/*static*/ void ModalShadowNode::OnModalClosed(IReactInstance &instance, int64_t tag) {
  folly::dynamic eventData = folly::dynamic::object("target", tag);
  instance.DispatchEvent(tag, "topRequestClose", std::move(eventData));

  folly::dynamic eventJson = folly::dynamic::object("target", tag);
  folly::dynamic params = folly::dynamic::array(tag, "onRquestClose", eventJson);
  instance.CallJsFunction("RCTEventEmitter", "receiveEvent", std::move(params));
}

void ModalShadowNode::UpdateLayout() {
  auto wkinstance = static_cast<ModalViewManager *>(GetViewManager())->m_wkReactInstance;
  auto instance = wkinstance.lock();

  if (instance == nullptr)
    return;

  auto popup = GetView().as<winrt::Popup>();

  // center relative to anchor
  if (m_targetTag > 0) {
    auto pNativeUIManagerHost = static_cast<NativeUIManager *>(instance->NativeUIManager())->getHost();
    ShadowNodeBase *pShadowNodeChild =
        static_cast<ShadowNodeBase *>(pNativeUIManagerHost->FindShadowNodeForTag(m_targetTag));

    if (pShadowNodeChild != nullptr) {
      auto targetView = pShadowNodeChild->GetView();
      auto targetElement = targetView.as<xaml::FrameworkElement>();

      auto popupTransform = targetElement.TransformToVisual(popup);
      winrt::Point bottomRightPoint(
          static_cast<float>(targetElement.Width()), static_cast<float>(targetElement.Height()));
      auto point = popupTransform.TransformPoint(bottomRightPoint);
    }
  } else // Center relative to app window
  {
    auto appWindow = xaml::Window::Current().Content();
    auto popupToWindow = appWindow.TransformToVisual(popup);
    auto appWindowSize = GetAppWindowSize();
    winrt::Point centerPoint;
    centerPoint.X = static_cast<float>((appWindowSize.Width - popup.ActualWidth()) / 2);
    centerPoint.Y = static_cast<float>((appWindowSize.Height - popup.ActualHeight()) / 2);
    auto point = popupToWindow.TransformPoint(centerPoint);
  }
}

void ModalShadowNode::UpdateTabStops() {
  auto control = GetControl();
  if (m_autoFocus) {
    control.IsTabStop(true);
    control.TabFocusNavigation(xaml::Input::KeyboardNavigationMode::Cycle);
    xaml::Input::FocusManager::TryFocusAsync(control, xaml::FocusState::Programmatic);
  } else {
    control.IsTabStop(false);
    control.TabFocusNavigation(xaml::Input::KeyboardNavigationMode::Local);
  }
}

winrt::Size ModalShadowNode::GetAppWindowSize() {
  winrt::Size windowSize = winrt::SizeHelper::Empty();

  if (auto current = xaml::Window::Current()) {
    if (auto coreWindow = current.CoreWindow()) {
      windowSize.Width = coreWindow.Bounds().Width;
      windowSize.Height = coreWindow.Bounds().Height;
    }
  }

  return windowSize;
}

ModalViewManager::ModalViewManager(const std::shared_ptr<IReactInstance> &reactInstance) : Super(reactInstance) {}

const char *ModalViewManager::GetName() const {
  return "RCTModalHostView";
}

folly::dynamic ModalViewManager::GetNativeProps() const {
  auto props = Super::GetNativeProps();

  props.update(folly::dynamic::object("animationType", "string"));
  //props.update(folly::dynamic::object("isOpen", "boolean")("isLightDismissEnabled", "boolean")("autoFocus", "boolean")(
  //    "horizontalOffset", "number")("verticalOffset", "number")("target", "number"));

  return props;
}

facebook::react::ShadowNode *ModalViewManager::createShadow() const {
  return new ModalShadowNode();
}

XamlView ModalViewManager::CreateViewCore(int64_t /*tag*/) {
  auto popup = winrt::Popup();
  winrt::Border border{};
  border.Background(xaml::Media::SolidColorBrush(ui::Colors::Magenta()));
  auto control = xaml::Controls::ContentControl();

  popup.Child(border);
  border.Child(control);

  popup.Loaded([popup](auto&&, auto &&) {
    winrt::FrameworkElement parent = popup;
    while (!parent.try_as<winrt::Microsoft::ReactNative::ReactRootView>()) {
      parent = parent.Parent().as<xaml::FrameworkElement>();
    }
    auto rootView = parent.as<winrt::Microsoft::ReactNative::ReactRootView>();
    rootView.Visibility(xaml::Visibility::Collapsed);
  });
  return popup;
}

void ModalViewManager::SetLayoutProps(
    ShadowNodeBase &nodeToUpdate,
    const XamlView &viewToUpdate,
    float left,
    float top,
    float width,
    float height) {
  auto& node = static_cast<ModalShadowNode&>(nodeToUpdate);
  auto popup = viewToUpdate.as<winrt::Popup>();
  auto control = node.GetControl();
  //popup.Child().as<xaml::Controls::ContentControl>();

  control.Width(width);
  control.Height(height);

  Super::SetLayoutProps(nodeToUpdate, viewToUpdate, left, top, width, height);
}

folly::dynamic ModalViewManager::GetExportedCustomDirectEventTypeConstants() const {
  auto directEvents = Super::GetExportedCustomDirectEventTypeConstants();
//  directEvents["topDismiss"] = folly::dynamic::object("registrationName", "onDismiss");
  directEvents["topRequestClose"] = folly::dynamic::object("registrationName", "onRequestClose");
  return directEvents;
}

} // namespace react::uwp
