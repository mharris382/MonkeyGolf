#pragma once

#include "Modules/ModuleManager.h"

class FVRGolfFeedbacks : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
