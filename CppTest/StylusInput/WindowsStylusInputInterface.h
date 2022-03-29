// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "IStylusInputModule.h"
#include <CoreMinimal.h>
#include "IStylusState.h"

class FWindowsStylusInputInterfaceImpl;
class SWindow;

class FWindowsStylusInputInterface
{
public:
	FWindowsStylusInputInterface(TUniquePtr<FWindowsStylusInputInterfaceImpl> InImpl);
	virtual ~FWindowsStylusInputInterface();

	virtual void Tick();
	virtual int NumInputDevices() const;
	virtual IStylusInputDevice* GetInputDevice(int Index) const;

private:
	// pImpl to avoid including Windows headers.
	TUniquePtr<FWindowsStylusInputInterfaceImpl> Impl;
	TArray<IStylusMessageHandler*> MessageHandlers;

	void CreateStylusPluginForHWND(void* HwndPtr);
	void RemovePluginForWindow(const TSharedRef<SWindow>& Window);
};
