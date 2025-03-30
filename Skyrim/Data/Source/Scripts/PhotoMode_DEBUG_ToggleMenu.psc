Scriptname PhotoMode_DEBUG_ToggleMenu extends activemagiceffect  
{Opens or closes the photo mode menu, with extra debug information. Meant for debugging.}

Bool Property bOpenMenu = True Auto
{Controls whether or not to open/close the menu. Default: True.}

Import po3_photomode

Event OnEffectStart(Actor a_kTarget, Actor a_kCaster)

    bool bPhotoModeOpen = IsPhotoModeActive()
    If (bPhotoModeOpen && bOpenMenu) 

        Debug.Messagebox("Photo mode already open, aborting.")
        Return
    EndIf

    If (!bPhotoModeOpen && !bOpenMenu)

        Debug.Messagebox("Photo mode already closed, aborting.")
        Return 
    EndIf

    TogglePhotoMode(bOpenMenu)
EndEvent