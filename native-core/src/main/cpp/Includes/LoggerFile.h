#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void LoggerFileSetEnabled(int enabled);
void LoggerFileSetPath(const char *path);
void LoggerFileAppend(int level, const char *tag, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
