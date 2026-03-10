#pragma once

#include "Modules/ModuleManager.h"

class FVRGolfOnline : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
