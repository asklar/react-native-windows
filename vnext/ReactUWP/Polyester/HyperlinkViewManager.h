// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "ContentControlViewManager.h"

namespace react {
namespace uwp {
namespace polyester {

class HyperlinkViewManager : public ContentControlViewManager {
  using Super = ContentControlViewManager;

 public:
  HyperlinkViewManager(const std::shared_ptr<IReactInstance> &reactInstance);

  const char *GetName() const override;
  folly::dynamic GetNativeProps() const override;
  folly::dynamic GetExportedCustomDirectEventTypeConstants() const override;

  bool UpdateProperty(
      ShadowNodeBase *nodeToUpdate,
      const std::string &propertyName,
      const folly::dynamic &propertyValue) override;

 protected:
  XamlView CreateViewCore(int64_t tag) override;
};

} // namespace polyester
} // namespace uwp
} // namespace react
