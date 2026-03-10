#pragma once

#include "Modules/ModuleManager.h"

class FVRGolfUI : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
