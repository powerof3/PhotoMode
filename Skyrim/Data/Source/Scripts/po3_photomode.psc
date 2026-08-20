Scriptname po3_photomode

;/
Returns a vector of ints corresponding to the version of the DLL.
eg: [3, 0, 0]
/;
Int[] Function GetVersion() Global Native

;/
Returns True if the PhotoMode UI is active, false otherwise.
/;
Bool Function IsPhotoModeActive() Global Native

;/
Attempts to open/close the PhotoMode menu, based on abOpen.
Returns true if successful, false if not.
/;
Bool Function TogglePhotoMode(Bool abOpen) Global Native 

;/
Returns True if the PhotoGallery UI is active, false otherwise.
/;
Bool Function IsPhotoGalleryActive() Global Native

;/
Attempts to open/close the PhotoGallery menu, based on abOpen.
Returns true if successful, false if not.
/;
Bool Function TogglePhotoGallery(Bool abOpen) Global Native 
