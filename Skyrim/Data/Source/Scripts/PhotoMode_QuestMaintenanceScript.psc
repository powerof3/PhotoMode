Scriptname PhotoMode_QuestMaintenanceScript extends Quest  
{Maintenance script.}

Import po3_photomode

Int[] Property iVersion Auto Hidden
{Internal copy of the photo mode version, used to check for updates.}

Message Property PhotoMode_MSG_DLLMissing Auto 
Message Property PhotoMode_MSG_DLLIncompatible Auto 
Message Property PhotoMode_MSG_DLLNeedsUpdate Auto

;/
==========================================================
          Photomode + Tween Menu Integration
==========================================================
/;

;This isn't documented, but you can send the OpenMenuFromTween event
;and TweenMenu will pick it up.

Event OnPhotoModeShow(String eventName, String strArg, Float numArg, Form sender)
    If (eventName != "OpenTween_PhotoMode")

        Return
    EndIf

    Int iMaxAttempts = 20
    While (iMaxAttempts > 0 && Utility.IsInMenuMode())

        iMaxAttempts -= 1
        Utility.Wait(0.25)
    EndWhile

    Utility.Wait(1.0)

    If (iMaxAttempts < 0)

        Return
    EndIf

    If (IsPhotoModeActive())

        Return
    EndIf

    ToggleMenu(True)
EndEvent

;/
==========================================================
                  End of Integration
==========================================================
/;

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

    RegisterForModEvent("OpenTween_PhotoMode", "OnPhotoModeShow")
    Return 0
EndFunction

Event OnInit()

    Maintenance()
EndEvent