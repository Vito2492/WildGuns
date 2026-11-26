package com.vito2492.wildguns;

import org.libsdl.app.SDLActivity;

/**
 * SDL2 game activity. Launched by RomPickerActivity once rom.sfc is validated.
 * Supports direct-touch crosshair aiming, on-screen multi-touch controls,
 * Bluetooth/USB gamepads, and Android Back button.
 */
public class WildGunsActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "main" };
    }
}
