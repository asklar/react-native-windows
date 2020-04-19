/**
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

import HomePage from '../pages/HomePage';
// import { By } from '../pages/BasePage';
// import ImageTestPage from '../pages/ImageTestPage';
import assert from 'assert';
// import WebdriverIO from 'webdriverio';

beforeAll(() => {
  HomePage.loadRNTester();
  Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, 3000);
  HomePage.waitForPageLoaded();
  // HomePage.clickAndGotoImagePage();
});

describe('basicTest', () => {
  it('basicTest', () => {

    const port = $$('x_DebuggerPort');
    assert(port);
    // assert(`port = ${port.getText()}`);

    const treedumpText = HomePage.treeDump();
    assert(treedumpText == "tree dump goes here");
    // const treedump = By('x_TreeDump');
    // const size = treedump.getSize();
    // assert(size.width == 20, `size = ${size}`);
  });
});
// describe('ImageWithoutBorderTest', () => {
//   /* Test case #1: view and image displayed with no border and cornerRadius */
//   it('ImageWithoutBorderTest', () => {
//     const result = ImageTestPage.getTreeDumpResult();
//     assert(result, '#1. Dump comparison for image without border!');
//   });

//   /* Test case #2: Click button once, update view and image with round border*/
//   it('ImageWithBorderTest', () => {
//     ImageTestPage.toggleImageBorder();
//     const result = ImageTestPage.getTreeDumpResult();
//     assert(result, '#2. Dump comparison for image with border!');
//   });

//   /* Test case #3: Click button one more, remove border from view and image but tree sturcture is different from #1*/
//   it('ImageWithoutBorderTest', () => {
//     ImageTestPage.toggleImageBorder();
//     const result = ImageTestPage.getTreeDumpResult();
//     assert(result, '#3. Second dump comparison for image without border!');
//   });

//   it('ImageRTLTest', () => {
//     ImageTestPage.toggleRTLMode();
//     const result = ImageTestPage.getTreeDumpResult();
//     assert(result, '#4. Dump comparison for image RTL');
//   });
// });
