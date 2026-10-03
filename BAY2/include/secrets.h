#ifndef SECRETS_H
#define SECRETS_H

// =====================================================================
// EV CHARGING STATION OPTIMIZER - SECRETS TEMPLATE
//
// 1. Copy this file to "secrets.h" in the same folder (include/).
// 2. Fill in the real values below for THIS bay only.
// 3. secrets.h must be listed in .gitignore and must NEVER be
//    committed to a public (or private) GitHub repository.
//
// This file (secrets.example.h) IS committed, with placeholders only,
// so anyone cloning the repo knows what to fill in.
// =====================================================================

#define SECRET_WIFI_SSID   "Wokwi-GUEST"
#define SECRET_WIFI_PASS   ""

// Paste the ThingsBoard device ACCESS TOKEN for THIS specific bay.
// Bay1, Bay2 and Bay3 each have their OWN token - never reuse one
// token across multiple devices; ThingsBoard treats the token as the
// device's identity, so a shared token makes two physical bays look
// like a single device.
#define SECRET_TB_TOKEN    "//ENTER-YOUR-THINGSBOARD-ACCESS-TOKEN-HERE (INSIDE THE INVERTED QUOTES)"     

// One of: "BAY1", "BAY2", "BAY3"
#define SECRET_BAY_ID      "BAY2"
//For Bay2, and Bay3, copy this complete file into the file explorer, open it, and change the SECRET_BAY_ID to "BAY2" or "BAY3" respectively, and also change the SECRET_TB_TOKEN to the correct token for that bay.

#endif