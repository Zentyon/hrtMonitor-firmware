#pragma once

#ifdef UI_MIRROR
void webMirrorInit();
void webMirrorLoop();
char webMirrorGetKey();
void mirrorFillScreen(uint16_t color);
void mirrorDrawString(const char* text, int x, int y, uint8_t size, uint16_t color);
void mirrorDrawLine(int x1, int y1, int x2, int y2, uint16_t color);
void mirrorDrawHLine(int x, int y, int w, uint16_t color);
#else
inline void webMirrorInit() {}
inline void webMirrorLoop() {}
inline char webMirrorGetKey() { return '\0'; }
inline void mirrorFillScreen(uint16_t) {}
inline void mirrorDrawString(const char*, int, int, uint8_t, uint16_t) {}
inline void mirrorDrawLine(int, int, int, int, uint16_t) {}
inline void mirrorDrawHLine(int, int, int, uint16_t) {}
#endif
