Scriptname po3_photomode

;/
Returns a vector of ints corresponding to the version of the DLL.
eg: [1, 9, 0]
/;
Int[] Function GetVersion() Global Native

;/
Returns True if the PhotoMode UI is active, false otherwise.
/;
Bool Function IsPhotoModeActive() Global Native

;/
Attempts to open/close the PhotoMode menu, based on a_bOpen.
Returns true if successful, false if not.
/;
Bool Function ToggleMenu(Bool a_bOpen) Global Native 
