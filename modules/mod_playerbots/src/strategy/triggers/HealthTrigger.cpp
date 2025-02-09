#include "HealthTrigger.h"

#include "Playerbots.h"

bool DeadTrigger::IsActive()
{
	return AI_VALUE2(bool, "dead", GetTargetName());
}