APP_NAME := MagicMouseBooster
BUNDLE_ID := local.wangfusen.MagicMouseBooster
SIGN_IDENTITY ?= -
-include Local.mk

BUILD_DIR := build
APP_BUNDLE := $(BUILD_DIR)/$(APP_NAME).app
CONTENTS := $(APP_BUNDLE)/Contents
MACOS_DIR := $(CONTENTS)/MacOS
VERSION := $(shell /usr/libexec/PlistBuddy -c "Print :CFBundleShortVersionString" Resources/Info.plist)
DIST_DIR := dist
PACKAGE_BUILD_DIR := $(DIST_DIR)/build
PACKAGE_ROOT := $(DIST_DIR)/dmg-root
PACKAGE_APP := $(PACKAGE_ROOT)/Magic Mouse Booster.app
DMG := $(DIST_DIR)/MagicMouseBooster-$(VERSION).dmg
SDK := $(shell xcrun --sdk macosx --show-sdk-path)
TARGET := arm64-apple-macosx26.0

CFLAGS := -std=c11 -O2 -Wall -Wextra -Werror -fblocks -arch arm64 \
	-isysroot $(SDK) -mmacosx-version-min=26.0 -I Sources/Core
OBJCFLAGS := -O2 -Wall -Wextra -Werror -fobjc-arc -arch arm64 \
	-isysroot $(SDK) -mmacosx-version-min=26.0 -I Sources/Core

CORE_OBJECTS := \
	$(BUILD_DIR)/PinchRecognizer.o \
	$(BUILD_DIR)/TapRecognizer.o \
	$(BUILD_DIR)/ControlScrollRecognizer.o \
	$(BUILD_DIR)/ScrollEventNormalizer.o \
	$(BUILD_DIR)/MouseGestureEngine.o

SWIFT_SOURCES := \
	Sources/App/AppDelegate.swift \
	Sources/App/GestureEngine.swift \
	Sources/App/SettingsViewController.swift \
	Sources/App/main.swift

.PHONY: all test probe verify package clean

all: $(APP_BUNDLE)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/PinchRecognizer.o: Sources/Core/PinchRecognizer.c Sources/Core/PinchRecognizer.h | $(BUILD_DIR)
	clang $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/TapRecognizer.o: Sources/Core/TapRecognizer.c Sources/Core/TapRecognizer.h Sources/Core/PinchRecognizer.h | $(BUILD_DIR)
	clang $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ControlScrollRecognizer.o: Sources/Core/ControlScrollRecognizer.c Sources/Core/ControlScrollRecognizer.h | $(BUILD_DIR)
	clang $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/ScrollEventNormalizer.o: Sources/Core/ScrollEventNormalizer.m Sources/Core/ScrollEventNormalizer.h | $(BUILD_DIR)
	clang $(OBJCFLAGS) -c $< -o $@

$(BUILD_DIR)/MouseGestureEngine.o: Sources/Core/MouseGestureEngine.c Sources/Core/MouseGestureEngine.h Sources/Core/MultitouchSupport.h Sources/Core/ControlScrollRecognizer.h Sources/Core/PinchRecognizer.h Sources/Core/ScrollEventNormalizer.h Sources/Core/TapRecognizer.h | $(BUILD_DIR)
	clang $(CFLAGS) -c $< -o $@

$(APP_BUNDLE): $(CORE_OBJECTS) $(SWIFT_SOURCES) Sources/App/BridgingHeader.h Resources/Info.plist Makefile
	mkdir -p $(MACOS_DIR)
	cp Resources/Info.plist $(CONTENTS)/Info.plist
	swiftc -O -warnings-as-errors -sdk $(SDK) -target $(TARGET) \
		-module-cache-path $(BUILD_DIR)/ModuleCache \
		-import-objc-header Sources/App/BridgingHeader.h \
		-I Sources/Core $(SWIFT_SOURCES) $(CORE_OBJECTS) \
		-o $(MACOS_DIR)/$(APP_NAME) \
		-framework AppKit -framework ApplicationServices -framework IOKit \
		-F /System/Library/PrivateFrameworks -framework MultitouchSupport
	codesign --force --sign $(SIGN_IDENTITY) --identifier $(BUNDLE_ID) $(APP_BUNDLE)

test: | $(BUILD_DIR)
	clang $(CFLAGS) Sources/Core/PinchRecognizer.c Tests/PinchRecognizerTests.c \
		-o $(BUILD_DIR)/PinchRecognizerTests
	$(BUILD_DIR)/PinchRecognizerTests
	clang $(CFLAGS) Sources/Core/TapRecognizer.c Tests/TapRecognizerTests.c \
		-o $(BUILD_DIR)/TapRecognizerTests
	$(BUILD_DIR)/TapRecognizerTests
	clang $(CFLAGS) Sources/Core/ControlScrollRecognizer.c \
		Tests/ControlScrollRecognizerTests.c \
		-o $(BUILD_DIR)/ControlScrollRecognizerTests
	$(BUILD_DIR)/ControlScrollRecognizerTests

probe: | $(BUILD_DIR)
	clang $(CFLAGS) Tests/MagicMouseProbe.c \
		-o $(BUILD_DIR)/MagicMouseProbe \
		-framework CoreFoundation -framework IOKit \
		-F /System/Library/PrivateFrameworks -framework MultitouchSupport
	$(BUILD_DIR)/MagicMouseProbe

verify: all test
	plutil -lint $(CONTENTS)/Info.plist
	codesign --verify --deep --strict --verbose=2 $(APP_BUNDLE)

package:
	rm -rf $(PACKAGE_BUILD_DIR) $(PACKAGE_ROOT)
	$(MAKE) BUILD_DIR=$(PACKAGE_BUILD_DIR) SIGN_IDENTITY=- all
	mkdir -p $(PACKAGE_ROOT)
	cp -R $(PACKAGE_BUILD_DIR)/$(APP_NAME).app "$(PACKAGE_APP)"
	ln -s /Applications "$(PACKAGE_ROOT)/Applications"
	codesign --verify --deep --strict --verbose=2 "$(PACKAGE_APP)"
	hdiutil create -volname "Magic Mouse Booster" -srcfolder $(PACKAGE_ROOT) \
		-ov -format UDZO $(DMG)
	hdiutil verify $(DMG)
	rm -rf $(PACKAGE_BUILD_DIR) $(PACKAGE_ROOT)

clean:
	rm -rf $(BUILD_DIR)
