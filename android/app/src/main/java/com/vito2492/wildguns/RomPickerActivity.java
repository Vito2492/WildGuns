package com.vito2492.wildguns;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.widget.Toast;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.zip.CRC32;

/**
 * First-launch ROM gatekeeper for Android:
 * - If internal storage already contains a valid rom.sfc (1MB Wild Guns),
 *   launches WildGunsActivity (SDL2) immediately.
 * - Otherwise prompts the user with Android's Storage Access Framework (SAF)
 *   to pick their own legally dumped Wild Guns (USA) ROM.
 */
public class RomPickerActivity extends Activity {
    private static final int REQ_PICK_ROM = 1001;
    private static final int ROM_SIZE = 0x100000;
    private static final long ROM_CRC_USA = 0x085d8d14L;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        File romFile = new File(getFilesDir(), "rom.sfc");
        if (romFile.exists() && romFile.length() == ROM_SIZE) {
            launchGame();
            return;
        }
        promptForRom();
    }

    private void launchGame() {
        Intent intent = new Intent(this, WildGunsActivity.class);
        startActivity(intent);
        finish();
    }

    private boolean hasWildGunsHeader(byte[] raw, int off) {
        if (raw.length - off < 0x8000) return false;
        String title = new String(raw, off + 0x7FC0, 9, StandardCharsets.US_ASCII);
        return "WILD GUNS".equals(title);
    }

    private void promptForRom() {
        new AlertDialog.Builder(this)
            .setTitle("Wild Guns")
            .setMessage("Selecciona tu ROM dumpeada legalmente de Wild Guns (USA) (.sfc / .smc).\n\nSelect your legally dumped Wild Guns (USA) ROM (.sfc / .smc).")
            .setCancelable(false)
            .setPositiveButton("Seleccionar ROM", (d, w) -> {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("*/*");
                startActivityForResult(intent, REQ_PICK_ROM);
            })
            .setNegativeButton("Salir", (d, w) -> finish())
            .show();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQ_PICK_ROM) return;
        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            promptForRom();
            return;
        }
        try {
            Uri uri = data.getData();
            InputStream in = getContentResolver().openInputStream(uri);
            if (in == null) { promptForRom(); return; }
            ByteArrayOutputStream buf = new ByteArrayOutputStream();
            byte[] tmp = new byte[16384];
            int n;
            while ((n = in.read(tmp)) > 0) buf.write(tmp, 0, n);
            in.close();
            byte[] raw = buf.toByteArray();
            int off = (raw.length == ROM_SIZE + 512) ? 512 : 0; // strip 512-byte SMC copier header
            if (raw.length - off != ROM_SIZE) {
                Toast.makeText(this, "ROM invalida: debe ser Wild Guns (SNES) de 1 MB.", Toast.LENGTH_LONG).show();
                promptForRom();
                return;
            }
            CRC32 crc = new CRC32();
            crc.update(raw, off, ROM_SIZE);
            if (crc.getValue() != ROM_CRC_USA && !hasWildGunsHeader(raw, off)) {
                Toast.makeText(this, "ROM no reconocida como Wild Guns (USA).", Toast.LENGTH_LONG).show();
                promptForRom();
                return;
            }
            File romFile = new File(getFilesDir(), "rom.sfc");
            try (FileOutputStream fos = new FileOutputStream(romFile)) {
                fos.write(raw, off, ROM_SIZE);
            }
            launchGame();
        } catch (Exception e) {
            Toast.makeText(this, "Error leyendo la ROM: " + e.getMessage(), Toast.LENGTH_LONG).show();
            promptForRom();
        }
    }
}
