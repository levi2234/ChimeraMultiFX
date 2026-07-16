import { defineConfig } from 'vite';
import { resolve } from 'node:path';

const outDir = resolve(import.meta.dirname, '../firmware/esp32/data');

export default defineConfig({
  base: '/',
  esbuild: {
    jsx: 'automatic',
    jsxImportSource: 'preact',
  },
  build: {
    outDir,
    emptyOutDir: true,
    cssCodeSplit: false,
    sourcemap: false,
    target: 'es2018',
    rollupOptions: {
      output: {
        entryFileNames: 'assets/app-[hash].js',
        assetFileNames: 'assets/[name]-[hash][extname]',
        manualChunks: undefined,
      },
    },
  },
});
