Scriptname PhotoMode_RefAliasLoadMonitor extends ReferenceAlias  
{Detects game loads and pings the maintenance quest.}

Event OnPlayerLoadGame()

    (Self.GetOwningQuest() As PhotoMode_QuestMaintenanceScript).Maintenance()
EndEvent