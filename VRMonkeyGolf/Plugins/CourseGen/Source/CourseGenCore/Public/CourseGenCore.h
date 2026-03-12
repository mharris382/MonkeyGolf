#pragma once

#include "Modules/ModuleManager.h"

class FCourseGenCore : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
