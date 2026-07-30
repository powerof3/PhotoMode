# Changements IGCSDOF V20 — annotations techniques

## 1. Nouveau bridge direct

### `src/IGCSBridge/Bridge.h`
Déclare le singleton chargé de :

- découvrir `IgcsConnector.addon64` dynamiquement ;
- publier position, FOV, quaternion, matrice de vue et axes caméra ;
- recevoir les commandes exportées par IGCSDOF ;
- sauvegarder/restaurer la caméra au début et à la fin d'une session ;
- appliquer les déplacements multishot et panorama.

### `src/IGCSBridge/Bridge.cpp`
Implémente le bridge et exporte l'ABI attendue :

```cpp
IGCS_StartScreenshotSession
IGCS_EndScreenshotSession
IGCS_MoveCameraMultishot
IGCS_MoveCameraPanorama
```

La correction V20 essentielle est `BuildBasis()` : la base est reconstruite selon
la convention native découverte dans Photo Mode, équivalente à
`FromEulerAnglesZXY`. Les colonnes de la matrice sont :

```text
Right / Forward / Up
```

La rotation reste fixe pendant les samples. Seule la translation varie dans le
plan formé par `Right` et `Up`.

## 2. Hook du rendu caméra

### `src/Hooks.cpp`
Ajoute un hook vtable sur `RE::FreeCameraState::GetTranslation`.

Flux :

```text
Skyrim calcule la translation native
→ le bridge reçoit la valeur native
→ IGCSDOF remplace uniquement la translation rendue pendant une session
→ la rotation et le FOV natifs restent intacts
```

Le hook existant `FromEulerAnglesZXY` de Photo Mode est conservé. Il reste la
source de vérité pour la convention de rotation.

## 3. Cycle de vie Photo Mode

### `src/PhotoMode/Manager.cpp`
Trois appels sont ajoutés :

- activation : connexion et publication initiale ;
- frame update : publication continue de la caméra ;
- désactivation : fin de session, restauration et nettoyage du buffer.

Le bridge n'est donc disponible que lorsque le Photo Mode est réellement actif.

## 4. Build CMake

### `cmake/sourcelist.cmake`
Ajoute :

```cmake
src/IGCSBridge/Bridge.cpp
```

### `cmake/headerlist.cmake`
Ajoute :

```cmake
src/IGCSBridge/Bridge.h
```

## 5. Logs

La version GitHub proposée conserve un statut minimal dans :

```text
%TEMP%\Skyrim_IGCSDOF_status.txt
```

Les diagnostics lourds sont toujours présents mais désactivés par :

```cpp
constexpr bool kVerboseDiagnostics = false;
```

Cela permet de les réactiver sans réintroduire l'ancien code de test.

## 6. Validation effectuée

La V20 a été validée sur plusieurs renders avec :

- translation appliquée sans erreur ;
- aucune dérive de rotation ;
- aucune dérive de FOV ;
- axes ZXY orthonormaux ;
- focus visuellement stable en plongée et dans plusieurs orientations.

## 7. Non inclus dans ce commit

Le redimensionnement automatique de l'interface pendant le hotsampling doit être
traité dans un commit séparé. Il est indépendant du bridge caméra et sera plus
facile à relire ou proposer en pull request séparément.
