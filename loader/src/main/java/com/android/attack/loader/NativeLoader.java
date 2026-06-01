package com.android.attack.loader;

public final class NativeLoader {
    static {
        System.loadLibrary("loader");
    }

    private NativeLoader() {
    }

    public static boolean loadFromDir(String nativeLibraryDir, String libName) {
        return nativeLoadFromDir(nativeLibraryDir, libName);
    }

    public static boolean loadDownloaded(String absolutePath) {
        return nativeLoadDownloaded(absolutePath);
    }

    private static native boolean nativeLoadFromDir(String nativeLibraryDir, String libName);

    private static native boolean nativeLoadDownloaded(String absolutePath);
}
