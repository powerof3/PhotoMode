Scriptname PhotoMode_QuestMaintenanceScript extends Quest  
{Maintenance script.}

Import po3_photomode

Int[] Property iVersion Auto Hidden
{Internal copy of the photo mode version, used to check for updates.}

Message Property PhotoMode_MSG_DLLMissing Auto 
Message Property PhotoMode_MSG_DLLIncompatible Auto 
Message Property PhotoMode_MSG_DLLNeedsUpdate Auto

;/
Returns:
-1 Missing DLL
0  Nominal
1  Performing update
2  Incompatible update.
/;
Int Function Maintenance()

    If (!iVersion) 

        iVersion = GetVersion()

        If (!iVersion)
        
            PhotoMode_MSG_DLLMissing.Show()
            Return -1
        EndIf
    EndIf

    If (iVersion[0] < 1)

        PhotoMode_MSG_DLLIncompatible.Show()
        Self.Stop() ;Prevents the user from simply ignoring the message.
        Return 2
    EndIf

    If (iVersion[1] < 9)

        ;TODO: Maybe implement this?
        ;PhotoMode_MSG_DLLNeedsUpdate.Show()
        Self.Stop()
        Return 1
    EndIf

    Return 0
EndFunction

Event OnInit()

    Maintenance()
EndEvent